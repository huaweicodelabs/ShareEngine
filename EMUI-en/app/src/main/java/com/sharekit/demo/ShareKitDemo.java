/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 */

package com.sharekit.demo;

import static android.Manifest.permission.READ_EXTERNAL_STORAGE;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.content.FileProvider;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * sharekit test demo
 *
 * @since 2019-11-11
 */
public class ShareKitDemo extends Activity implements Button.OnClickListener {
    private static final String TAG = "ShareKitDemo";

    private static final String APP_PROVIDER = "com.sharekit.demo.onestep.provider";

    private static final String SHARE_PKG = "com.huawei.android.instantshare";

    private static final String SHARE_INTENT_TYPE = "*";

    private TextView shareType;

    private RadioButton radioText;

    private EditText shareText;

    private void initViews() {
        radioText = findViewById(R.id.radio_text);
        shareType = findViewById(R.id.share_type);
        shareText = findViewById(R.id.sharetext);
        Button cancel = findViewById(R.id.intent);
        cancel.setOnClickListener(this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sharekit_demo);
        initViews();
    }

    @Override
    protected void onResume() {
        super.onResume();
        checkPermission();
    }

    private void checkPermission() {
        if (checkSelfPermission(READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            String[] permissions = {READ_EXTERNAL_STORAGE};
            requestPermissions(permissions, 0);
        }
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
            case R.id.intent:
                onStartIntent();
                break;
            default:
                Log.w(TAG, "Invalid view clicked: " + v.getId());
                break;
        }
    }

    private void onStartIntent() {
        if (radioText.isChecked()) {
            doStartTextIntent();
        } else {
            doStartFileIntent();
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

    private void doStartTextIntent() {
        String text = shareText.getText().toString();
        if (TextUtils.isEmpty(text)) {
            return;
        }
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setType(SHARE_INTENT_TYPE);
        intent.putExtra(Intent.EXTRA_TEXT, "test text");
        intent.setPackage(SHARE_PKG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PackageManager manager = getApplicationContext().getPackageManager();
        List<ResolveInfo> infos = manager.queryIntentActivities(intent, 0);
        if (infos.size() > 0) {
            // size == 0 Indicates that the current device does not support Intent-based sharing
            getApplicationContext().startActivity(intent);
        }
    }

    private void doStartFileIntent() {
        String text = shareText.getText().toString();
        if (TextUtils.isEmpty(text)) {
            return;
        }
        ArrayList<Uri> uris = getFileUris(text);
        if (uris.isEmpty()) {
            return;
        }

        Intent intent;
        if (uris.size() == 1) {
            // Sharing a file
            intent = new Intent(Intent.ACTION_SEND);
            intent.setType(SHARE_INTENT_TYPE);
            intent.putExtra(Intent.EXTRA_STREAM, uris.get(0));
            intent.setPackage(SHARE_PKG);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } else {
            // Sharing multiple files
            intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
            intent.setType(SHARE_INTENT_TYPE);
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris);
            intent.setPackage(SHARE_PKG);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        }
        PackageManager manager = getApplicationContext().getPackageManager();
        List<ResolveInfo> infos = manager.queryIntentActivities(intent, 0);
        if (infos.size() > 0) {
            // size == 0 Indicates that the current device does not support Intent-based sharing
            getApplicationContext().startActivity(intent);
        }
    }
}
