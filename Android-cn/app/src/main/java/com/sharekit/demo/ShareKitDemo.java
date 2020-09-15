/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 */

package com.sharekit.demo;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.READ_EXTERNAL_STORAGE;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.content.FileProvider;
import android.text.TextUtils;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import com.huawei.android.hwshare.common.NearByDeviceEx;
import com.huawei.android.hwshare.ui.IWidgetCallback;
import com.huawei.connect.sharekit.common.ShareBean;
import com.huawei.connect.sharekit.constant.KitResult;
import com.huawei.connect.sharekit.manager.IShareKitInitCallback;
import com.huawei.connect.sharekit.manager.ShareKitManager;
import com.huawei.nearbysdk.DTCP.DTCPValueDef;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * sharekit test demo
 *
 * @since 2019-11-11
 */
public class ShareKitDemo extends Activity implements Button.OnClickListener {
    private static final String TAG = "ShareKitDemo";

    private static SimpleDateFormat format = new SimpleDateFormat("hh:mm:ss.SSS", Locale.ENGLISH);

    /* sharekit have default provider named ${applicationId}.onestep.provider */
    private static final String APP_PROVIDER = "com.sharekit.demo.onestep.provider";

    /* trans state chagne type: sending percentage */
    private static final int STATE_PROGRESS = 1;

    /* trans state chagne type: send finish */
    private static final int STATE_SUCCESS = 2;

    /* trans state chagne type: send status change */
    private static final int STATE_STATUS = 3;

    /* trans state chagne type: send failed */
    private static final int STATE_ERROR = 4;

    /* common device id is unique, two devices with same bluetooth name can be identified by common id */
    private static final int COMMON_ID_PRINT_LENGTH = 10;

    /* use sharekit abilities through ShareKitManager instance */
    private ShareKitManager shareKitManager = null;

    /* the devices set stored with common device id */
    private Map<String, NearByDeviceEx> deviceMap = new HashMap<>();

    /* the devices found time set stored with common device id */
    private Map<String, String> foundTimeMap = new HashMap<>();

    private final Object lock = new Object();

    private ArrayAdapter<String> deviceSelectAdapter;

    private ScrollView scrollView;

    private TextView deviceView;

    private TextView statusView;

    private TextView shareType;

    private RadioButton radioText;

    private EditText shareText;

    private EditText destDevice;

    /* ShareKit callback interface，will be called when device found or disappear, and status change */
    private IWidgetCallback callback = new IWidgetCallback.Stub() {
        @Override
        public synchronized void onDeviceFound(NearByDeviceEx nearByDeviceEx) {
            Log.i(TAG, "onDeviceFound");
            String item = getString(R.string.sharekit_device_found, nearByDeviceEx.getBtName()
                + "-" + nearByDeviceEx.getCommonDeviceId().substring(0, COMMON_ID_PRINT_LENGTH));
            updateStatusHistory(item);
            synchronized (lock) {
                deviceMap.put(nearByDeviceEx.getCommonDeviceId(), nearByDeviceEx);
                foundTimeMap.put(nearByDeviceEx.getCommonDeviceId(), format.format(new Date()));
            }
            updateDeviceList();
        }

        @Override
        public void onDeviceDisappeared(NearByDeviceEx nearByDeviceEx) {
            Log.i(TAG, "onDeviceDisappeared");
            String item = getString(R.string.sharekit_device_disappear, nearByDeviceEx.getBtName()
                + "-" + nearByDeviceEx.getCommonDeviceId().substring(0, COMMON_ID_PRINT_LENGTH));
            updateStatusHistory(item);
            synchronized (lock) {
                deviceMap.remove(nearByDeviceEx.getCommonDeviceId());
                foundTimeMap.remove(nearByDeviceEx.getCommonDeviceId());
            }
            updateDeviceList();
        }

        @Override
        public void onTransStateChange(NearByDeviceEx nearByDeviceEx, int state, int stateValue) {
            /* check the document about what does each number means */
            Log.i(TAG, "trans state:" + state + " value:" + stateValue);
            String stateDesc = "";
            switch (state) {
                case STATE_PROGRESS:
                    stateDesc = getString(R.string.sharekit_send_progress, stateValue);
                    break;
                case STATE_SUCCESS:
                    stateDesc = getString(R.string.sharekit_send_finish);
                    break;
                case STATE_STATUS:
                    stateDesc = getString(R.string.sharekit_state_chg, translateStateValue(stateValue));
                    break;
                case STATE_ERROR:
                    stateDesc = getString(R.string.sharekit_send_error, translateErrorValue(stateValue));
                    break;
                default:
                    break;
            }
            String item = getString(R.string.sharekit_trans_state, nearByDeviceEx.getBtName(), stateDesc);
            updateStatusHistory(item);
        }

        @Override
        public void onEnableStatusChanged() {
            int status = shareKitManager.getShareStatus();
            Log.i(TAG, "sharekit ability current status:" + status);
            updateStatusHistory(getString(R.string.sharekit_status), status);
        }
    };

    /* make sure app have these permissions, otherwise search and send can not work */
    private void checkPermission() {
        String[] permissions = {READ_EXTERNAL_STORAGE, ACCESS_FINE_LOCATION};
        List<String> requestPermissions = new ArrayList<>();
        for (String permission : permissions) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                requestPermissions.add(permission);
            }
        }
        if (!requestPermissions.isEmpty()) {
            requestPermissions(requestPermissions.toArray(new String[0]), 0);
        }
    }

    private void updateDeviceList() {
        synchronized (lock) {
            String deviceList = deviceMap.values()
                .stream()
                .map(device -> getString(R.string.sharekit_device_detail, device.getBtName(),
                    foundTimeMap.get(device.getCommonDeviceId())))
                .collect(Collectors.joining(System.lineSeparator()));
            runOnUiThread(() -> {
                deviceView.setText(getString(R.string.sharekit_device_list_detail, deviceMap.size(), deviceList));
                if (deviceSelectAdapter != null) {
                    deviceSelectAdapter.clear();
                    deviceSelectAdapter.addAll(
                        deviceMap.values().stream().map(NearByDeviceEx::getBtName).collect(Collectors.toList()));
                    deviceSelectAdapter.notifyDataSetChanged();
                }
            });
        }
    }

    private void clearDeviceList() {
        synchronized (lock) {
            deviceMap.clear();
            foundTimeMap.clear();
            runOnUiThread(() -> {
                deviceView.setText(R.string.sharekit_no_device);
                if (deviceSelectAdapter != null) {
                    deviceSelectAdapter.clear();
                    deviceSelectAdapter.notifyDataSetChanged();
                }
            });
        }
    }

    private void updateStatusHistory(String item) {
        String status = new StringBuilder(statusView.getText().toString()).append(System.lineSeparator())
            .append(item)
            .append(getString(R.string.sharekit_time) + format.format(new Date()))
            .toString();
        runOnUiThread(() -> {
            statusView.setText(status);
            scrollView.post(() -> scrollView.smoothScrollTo(0, statusView.getBottom()));
        });
    }

    private void updateStatusHistory(String item, int resultCode) {
        updateStatusHistory(item + translateResultCode(resultCode));
    }

    private String translateStateValue(int stateValue) {
        String stateDesc;
        switch (stateValue) {
            case DTCPValueDef.TCB_STATUS_SEND_PREVIEWOK:
                stateDesc = getString(R.string.sharekit_send_preview_ok);
                break;
            case DTCPValueDef.TCB_STATUS_SEND_DATA:
                stateDesc = getString(R.string.sharekit_send_data);
                break;
            case DTCPValueDef.TCB_STATUS_SEND_CANCEL:
                stateDesc = getString(R.string.sharekit_send_cancel);
                break;
            case DTCPValueDef.TCB_STATUS_SEND_RECVCANCEL:
                stateDesc = getString(R.string.sharekit_send_recv_cancel);
                break;
            case DTCPValueDef.TCB_STATUS_SEND_RECVREJCT:
                stateDesc = getString(R.string.sharekit_send_recv_reject);
                break;
            case DTCPValueDef.TCB_STATUS_SEND_RECVBUSY:
                stateDesc = getString(R.string.sharekit_send_recv_busy);
                break;
            case DTCPValueDef.TCB_STATUS_SEND_LOCALBUSY:
                stateDesc = getString(R.string.sharekit_send_local_busy);
                break;
            default:
                stateDesc = String.valueOf(stateValue);
                break;
        }
        return stateDesc;
    }

    private String translateErrorValue(int stateValue) {
        String stateDesc;
        switch (stateValue) {
            case DTCPValueDef.TCB_ERR_SOCKET:
                stateDesc = getString(R.string.sharekit_err_socket);
                break;
            case DTCPValueDef.TCB_ERR_REMOTE:
                stateDesc = getString(R.string.sharekit_err_remote);
                break;
            case DTCPValueDef.TCB_TIMEOUT_CONFIRM:
                stateDesc = getString(R.string.sharekit_timeout_confirm);
                break;
            case DTCPValueDef.TCB_TIMEOUT_FINISH:
                stateDesc = getString(R.string.sharekit_timeout_finish);
                break;
            case DTCPValueDef.TCB_TIMEOUT_FINISHOK:
                stateDesc = getString(R.string.sharekit_timeout_finishok);
                break;
            case DTCPValueDef.TCB_TIMEOUT_CANCEL:
                stateDesc = getString(R.string.sharekit_timeout_cancel);
                break;
            case DTCPValueDef.TCB_TIMEOUT_PREVIEWDATA:
                stateDesc = getString(R.string.sharekit_timeout_previewdata);
                break;
            case DTCPValueDef.TCB_TIMEOUT_FILEDATA:
                stateDesc = getString(R.string.sharekit_timeout_filedata);
                break;
            case DTCPValueDef.TCB_TIMEOUT_REJECT:
                stateDesc = getString(R.string.sharekit_timeout_reject);
                break;
            case DTCPValueDef.TCB_TIMEOUT_DTCPAUTH:
                stateDesc = getString(R.string.sharekit_timeout_dtcpauth);
                break;
            case DTCPValueDef.TCB_TIMEOUT_SOCKET_CONNECT:
                stateDesc = getString(R.string.sharekit_timeout_socket_connect);
                break;
            case DTCPValueDef.TCB_ERR_NOSPACE:
                stateDesc = getString(R.string.sharekit_err_nospace);
                break;
            case DTCPValueDef.TCB_ERR_IO:
                stateDesc = getString(R.string.sharekit_err_io);
                break;
            default:
                stateDesc = String.valueOf(stateValue);
                break;
        }
        return stateDesc;
    }

    private String translateResultCode(int resultcode) {
        String resultDesc;
        switch (resultcode) {
            case KitResult.SUCCESS:
                resultDesc = getString(R.string.sharekit_status_ok);
                break;
            case KitResult.ERROR_SERVICE_NOT_CONNECTED:
                resultDesc = getString(R.string.sharekit_service_not_connected);
                break;
            case KitResult.ERROR_SHARE_ABILITY_NOT_READY:
                resultDesc = getString(R.string.sharekit_ability_not_ready);
                break;
            case KitResult.ERROR_REMOTE_EXCEPTION:
                resultDesc = getString(R.string.sharekit_remote_exception);
                break;
            case KitResult.ERROR_BLUETOOTH_WIFI_PERMISSION:
                resultDesc = getString(R.string.sharekit_bluetooth_wifi_no_permission);
                break;
            case KitResult.ERROR_SERVICE_NOT_READY:
                resultDesc = getString(R.string.sharekit_service_not_inited);
                break;
            case KitResult.ERROR_PERMISSION_VERIFY_FAILED:
                resultDesc = getString(R.string.sharekit_permission_verify_failed);
                break;
            case KitResult.ERROR_INVALID_APPID:
                resultDesc = getString(R.string.sharekit_invalid_appid);
                break;
            case KitResult.ERROR_GET_APPID_FAILED:
                resultDesc = getString(R.string.sharekit_get_appid_failed);
                break;
            case KitResult.ERROR_INVALID_PARAMETER:
                resultDesc = getString(R.string.sharekit_invalid_parameter);
                break;
            case KitResult.ERROR_SIZE_OUT_OF_LIMIT:
                resultDesc = getString(R.string.sharekit_size_out_of_limit);
                break;
            default:
                resultDesc = String.valueOf(resultcode);
                break;
        }
        return resultDesc;
    }

    private void initViews() {
        destDevice = findViewById(R.id.dest);
        deviceView = findViewById(R.id.device_list);
        scrollView = findViewById(R.id.scrollview);
        statusView = findViewById(R.id.status_history);
        statusView.setMovementMethod(ScrollingMovementMethod.getInstance());
        statusView.setTextIsSelectable(true);
        radioText = findViewById(R.id.radio_text);
        shareType = findViewById(R.id.share_type);
        shareText = findViewById(R.id.sharetext);
        Spinner deviceList = findViewById(R.id.devices);
        deviceSelectAdapter = new ArrayAdapter<>(this, R.layout.simple_spinner_item);
        deviceSelectAdapter.setDropDownViewResource(R.layout.simple_spinner_dropdown_item);
        deviceList.setAdapter(deviceSelectAdapter);
        deviceList.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                destDevice.setText(deviceSelectAdapter.getItem(position));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                destDevice.setText("");
            }
        });

        Button init = findViewById(R.id.init);
        init.setOnClickListener(this);
        Button unInit = findViewById(R.id.uninit);
        unInit.setOnClickListener(this);
        Button register = findViewById(R.id.register);
        register.setOnClickListener(this);
        Button unregister = findViewById(R.id.unregister);
        unregister.setOnClickListener(this);
        Button discovery = findViewById(R.id.discovery);
        discovery.setOnClickListener(this);
        Button stopDiscovery = findViewById(R.id.stop_discovery);
        stopDiscovery.setOnClickListener(this);
        Button getStatus = findViewById(R.id.get_status);
        getStatus.setOnClickListener(this);
        Button getDeviceList = findViewById(R.id.get_device_list);
        getDeviceList.setOnClickListener(this);
        Button enable = findViewById(R.id.enable);
        enable.setOnClickListener(this);
        Button send = findViewById(R.id.send);
        send.setOnClickListener(this);
        Button cancel = findViewById(R.id.cancel);
        cancel.setOnClickListener(this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sharekit_demo);
        initViews();

        shareKitManager = new ShareKitManager(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        /* check permission when need */
        checkPermission();
    }

    @Override
    protected void onDestroy() {
        /* unregister the callback and uninit the shareKitManager */
        shareKitManager.unregisterCallback(callback);
        shareKitManager.uninit();
        super.onDestroy();
    }

    /**
     * switch between send text or files
     *
     * @param view the view
     */
    public void onRadioButtonSwitched(View view) {
        if (radioText.isChecked()) {
            shareType.setText(getString(R.string.sharekit_text_content));
        } else {
            shareType.setText(R.string.sharekit_file_list);
            shareText.setText(R.string.sharekit_file_example);
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.init:
                onInit();
                break;
            case R.id.uninit:
                onUninit();
                break;
            case R.id.register:
                onRegister();
                break;
            case R.id.unregister:
                onUnregister();
                break;
            case R.id.discovery:
                onDiscovery();
                break;
            case R.id.stop_discovery:
                onStopDiscovery();
                break;
            case R.id.get_status:
                onGetStatus();
                break;
            case R.id.get_device_list:
                onGetDeviceList();
                break;
            case R.id.enable:
                onEnable();
                break;
            case R.id.send:
                onSend();
                break;
            case R.id.cancel:
                onCancel();
                break;
            default:
                Log.w(TAG, "Invalid view clicked: " + v.getId());
                break;
        }
    }

    private void onInit() {
        IShareKitInitCallback initCallback = isSuccess -> {
            Log.i(TAG, "sharekit init result:" + isSuccess);
            updateStatusHistory(
                isSuccess ? getString(R.string.sharekit_init_finish) : getString(R.string.sharekit_init_failed));
        };
        updateStatusHistory(getString(R.string.sharekit_init_start));
        shareKitManager.init(initCallback);
        clearDeviceList();
    }

    private void onUninit() {
        shareKitManager.uninit();
        updateStatusHistory(getString(R.string.sharekit_uninit));
        clearDeviceList();
    }

    private void onRegister() {
        int ret = shareKitManager.registerCallback(callback);
        updateStatusHistory(getString(R.string.sharekit_register_ret), ret);
        clearDeviceList();
    }

    private void onUnregister() {
        int ret = shareKitManager.unregisterCallback(callback);
        updateStatusHistory(getString(R.string.sharekit_unregister_ret), ret);
        clearDeviceList();
    }

    private void onDiscovery() {
        int ret = shareKitManager.startDiscovery();
        updateStatusHistory(getString(R.string.sharekit_start_discovery_ret), ret);
        clearDeviceList();
    }

    private void onStopDiscovery() {
        int ret = shareKitManager.stopDiscovery();
        updateStatusHistory(getString(R.string.sharekit_stop_discover_ret), ret);
        clearDeviceList();
    }

    private void onGetStatus() {
        int ret = shareKitManager.getShareStatus();
        updateStatusHistory(getString(R.string.sharekit_status_ret), ret);
    }

    private void onGetDeviceList() {
        List<NearByDeviceEx> deviceList = shareKitManager.getDeviceList();
        String devices = deviceList.stream().map(NearByDeviceEx::getBtName).collect(Collectors.joining(" "));
        updateStatusHistory(getString(R.string.sharekit_get_device_list_ret, deviceList.size(), devices));
    }

    private void onEnable() {
        int ret;
        ret = shareKitManager.enable();
        updateStatusHistory(getString(R.string.sharekit_enable_ret), ret);
    }

    private void onSend() {
        if (radioText.isChecked()) {
            doSendText();
        } else {
            doSendFiles();
        }
    }

    private void onCancel() {
        String deviceName = destDevice.getText().toString();
        if (TextUtils.isEmpty(deviceName)) {
            return;
        }
        updateStatusHistory(getString(R.string.sharekit_start_found, deviceName));
        synchronized (lock) {
            List<NearByDeviceEx> deviceList = shareKitManager.getDeviceList();
            for (NearByDeviceEx device : deviceList) {
                if (deviceName.equals(device.getBtName())) {
                    int ret = shareKitManager.cancelSend(device);
                    updateStatusHistory(getString(R.string.sharekit_device_desc, deviceName,
                        device.getCommonDeviceId().substring(0, COMMON_ID_PRINT_LENGTH)));
                    updateStatusHistory(getString(R.string.sharekit_try_cancel_ret, deviceName), ret);
                }
            }
        }
    }

    private void doSendText() {
        String text = shareText.getText().toString();
        if (TextUtils.isEmpty(text)) {
            updateStatusHistory(getString(R.string.sharekit_content_empty));
            return;
        }
        String deviceName = destDevice.getText().toString();
        if (TextUtils.isEmpty(deviceName)) {
            updateStatusHistory(getString(R.string.sharekit_device_name_empty));
            return;
        }

        ShareBean shareBean = new ShareBean(text);
        doSend(deviceName, shareBean);
    }

    private void doSendFiles() {
        String text = shareText.getText().toString();
        if (TextUtils.isEmpty(text)) {
            updateStatusHistory(getString(R.string.sharekit_file_list_empty));
            return;
        }
        String deviceName = destDevice.getText().toString();
        if (TextUtils.isEmpty(deviceName)) {
            updateStatusHistory(getString(R.string.sharekit_device_name_empty));
            return;
        }

        List<Uri> uris = getFileUris(text);
        if (uris.isEmpty()) {
            updateStatusHistory(getString(R.string.sharekit_no_files_left));
            return;
        }

        ShareBean shareBean = new ShareBean(uris);
        doSend(deviceName, shareBean);
    }

    private void doSend(String deviceName, ShareBean shareBean) {
        updateStatusHistory(getString(R.string.sharekit_start_found, deviceName));
        synchronized (lock) {
            boolean isDeviceFound = false;
            for (NearByDeviceEx device : deviceMap.values()) {
                if (deviceName.equals(device.getBtName())) {
                    int ret = shareKitManager.doSend(device, shareBean);
                    updateStatusHistory(getString(R.string.sharekit_device_desc, deviceName,
                        device.getCommonDeviceId().substring(0, COMMON_ID_PRINT_LENGTH)));
                    updateStatusHistory(getString(R.string.sharekit_try_send, deviceName), ret);
                    isDeviceFound = true;
                }
            }
            if (!isDeviceFound) {
                updateStatusHistory(getString(R.string.sharekit_device_alread_lost));
            }
        }
    }

    private ArrayList<Uri> getFileUris(String text) {
        ArrayList<Uri> uris = new ArrayList<>();
        File storageDirectory = Environment.getExternalStorageDirectory();
        String[] paths = text.split(System.lineSeparator());
        for (String item : paths) {
            File filePath = new File(storageDirectory, item);
            if (!filePath.isDirectory()) {
                if (!filePath.exists()) {
                    updateStatusHistory(getString(R.string.sharekit_file_not_exist, item));
                    continue;
                }
                uris.add(FileProvider.getUriForFile(this, APP_PROVIDER, filePath));
                continue;
            }
            File[] files = filePath.listFiles();
            for (File file : files) {
                if (file.isFile()) {
                    uris.add(FileProvider.getUriForFile(this, APP_PROVIDER, file));
                }
            }
        }
        return uris;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }
}
