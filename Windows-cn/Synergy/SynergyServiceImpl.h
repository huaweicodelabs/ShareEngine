#pragma once

#include "cstringt.h"
#include "atlstr.h"
#include <vector>

using namespace std;

void StopSynergyService();
void UnInitSynergyService();


#define WIN_SHARE_KIT_SERVER_API __declspec(dllimport)


// 接口定义，定义插件需要提供的一些接口
// clsid 指定组件类的全局唯一标识信息，clsid规则：进程名.插件名
// 注意：clsid 不可重复（复制已有接口文件时容易发生），否则将会覆盖已向管理器注册的类，导致其他模块无法创建原来类的接口对象。

const char* const plnComponentClsid = "PluginsLoadApp.SynergyServer";		// clsid规则：进程名.插件名


namespace WinShareKit {
    enum HWSHARE_CMD
    {
        CANCEL = 0,
        REJECT = 1,
    };

    enum HWSHARE_STATUS
    {
        SENDING_FILES_PROCESS = 0,

    };

    struct HwShareProgress
    {
        int32_t progress;
        int32_t secRemains;
        int32_t speed;
    };

    union HwShareStatus
    {
        HwShareStatus()
            : longStatus(0)
        {
        }
        ~HwShareStatus() {};
        long long longStatus;
        int32_t intStatus;
        float fltStatus;
        double dlbStatus;
        size_t sizeStatus;
        char cStatus[8];
    };

    struct HwShareDevice
    {
        int32_t uniqueKey = 0;
        int32_t IsPC = 0;//0: not PC; 1: PC
        int32_t isPaired = 0;//是否配对设备
        int32_t shareType = 0;//0:hwshare分享; 1:蓝牙分享
        int64_t nMacAddr = 0;//蓝牙Mac地址
                             //std::string strMacAddr = "";//蓝牙Mac地址
        std::string NickName;
        std::string Summary;
        std::string DevName;
        std::string UserName;
        int32_t isWifi5GAbility = true;
        int32_t isAuthored = 0;
    };

    enum HWShareDTCP
    {
        SHARE_SUCCESS = 0,
        SHARE_OK,

        SHARE_ERROR_BASE,
        SHARE_INVALID_PARAM,
        SHARE_INVALID_OPERATION,
        SHARE_ERROR_IO,
        SHARE_ERROR_UNSUPPORTED,
        SHARE_ERROR_BUSY,
        SHARE_ERROR_INIT,
        SHARE_ERROR_SOCKET,
        SHARE_ERROR_TIMEOUT,
        SHARE_ERROR_REMOTE,
        SHARE_ERROR_PUBLISH,
        SHARE_ERROR_SUBSCRIBE,
        SHARE_ERROR_UNPUBLISH,
        SHARE_ERROR_UNSUBSCRIBE,

        SHARE_ERROR_SERVER_BUSY,
        SHARE_ERROR_CLIENT_BUSY,
        SHARE_ERROR_SELF_CHANNEL_BUSY,
        SHARE_ERROR_OTHER_CHANNEL_BUSY,

        SHARE_UNKNOWN_ERROR,

        RTN_ERROR_END,

        /////////////////////////////////////////////////////////////////////////////////
        DTCP_ERR_BASE,
        DTCP_ERR_SOCKET,
        DTCP_ERR_MISSED_PAIR_DEVICE,
        DTCP_ERR_REMOTE,
        DTCP_ERR_VERSION_MISMATCH,
        DTCP_ERR_NOSPACE,
        DTCP_ERR_IO,

        DTCP_ERR_UNKNOWN,

        DTCP_ERR_END,

        DTCP_TIMEOUT_BASE,
        // Preview data send complete, but get confirm or cancel,reject message timeout
        DTCP_TIMEOUT_CONFIRM,

        // Data send complete, but not get finish message from receiver
        DTCP_TIMEOUT_FINISH,

        // data receive complete, but not get finish response message from sender
        // In this case ITransmitCallback.onSuccess method still be called
        DTCP_TIMEOUT_FINISHOK,

        // Send cancel command,but not get response from remote
        DTCP_TIMEOUT_CANCEL,

        // Data socket connetc, but not receive any preview data in time
        DTCP_TIMEOUT_PREVIEWDATA,

        // Send confirm command, but not receive any file data in time
        DTCP_TIMEOUT_FILEDATA,
        DTCP_TIMEOUT_END,

        //////////////////////////////////////////////////////////////////////////////////
        DTCP_STATUS_SEND_BASE,

        // connect to receiver
        DTCP_STATUS_SEND_CONNECTING,

        // connect to receiver successed
        DTCP_STATUS_SEND_CONNECTED,

        // start send preview data
        DTCP_STATUS_SEND_PREVIEW,

        // send preview data complete
        DTCP_STATUS_SEND_PREVIEWOK,

        // start send file data
        DTCP_STATUS_SEND_DATA,

        // send file data complete
        DTCP_STATUS_SEND_DATAOK,

        // disconnect socket
        DTCP_STATUS_SEND_DISCONNECTED,

        // receiver cancel
        DTCP_STATUS_SEND_RECVCANCEL,

        // sender cancel
        DTCP_STATUS_SEND_CANCEL,

        DTCP_STATUS_SEND_CANCELING,

        // receiver rejct
        DTCP_STATUS_SEND_RECVREJCT,

        // send emtpy data
        DTCP_STATUS_SEND_EMTPY,

        // send finish ok
        DTCP_STATUS_SEND_SUCCESS,

        // there are other send mission, wait for complete
        DTCP_STATUS_SEND_WAITING,

        DTCP_STATUS_SEND_END,

        ////////////////////////////////////////////////////////////////////////////////////////////////
        DTCP_STATUS_RECV_BASE,

        // start receive file data
        DTCP_STATUS_RECV_DATA,

        // receive file data complete
        DTCP_STATUS_RECV_DATAOK,

        // disconnect socket
        DTCP_STATUS_RECV_DISCONNECTED,

        // sender cancel
        DTCP_STATUS_RECV_SENDCANCEL,

        // receiver cancel
        DTCP_STATUS_RECV_CANCEL,

        DTCP_STATUS_RECV_REJECT,

        // Empty Data
        DTCP_STATUS_RECV_EMTPYDATA,

        // receive finish ok
        DTCP_STATUS_RECV_SUCCESS,

        // BT or WiFi Close
        DTCP_BT_OR_WIFI_CLOSE,

        // Wifi exception disconnect
        DTCP_WIFI_EXCEPTION_DISCONNECT
    };

    class WIN_SHARE_KIT_SERVER_API DeviceChangeListener
    {
    public:
        virtual ~DeviceChangeListener() {};
        virtual void OnDeviceFound(const WinShareKit::HwShareDevice& device) = 0;
        virtual void OnDeviceLost(const WinShareKit::HwShareDevice& device) = 0;
    };

    class WIN_SHARE_KIT_SERVER_API NotifyFileRecvListener
    {
    public:
        virtual ~NotifyFileRecvListener() {};
        virtual void NotifyRecv(WinShareKit::HwShareDevice *device, const std::vector<std::string> files, int64_t totalSize, int32_t auth, const char *thumbnail, int32_t thumbnailSize) = 0;
    };

    class WIN_SHARE_KIT_SERVER_API RecvProgressListener
    {
    public:
        virtual ~RecvProgressListener() {};
        virtual void OnProgress(const WinShareKit::HwShareProgress& progress) = 0;
        virtual void OnStatusChange(const WinShareKit::HwShareStatus& status) = 0;
    };

    class WIN_SHARE_KIT_SERVER_API SendProgressListener
    {
    public:
        virtual ~SendProgressListener() {};
        virtual void OnProgress(const WinShareKit::HwShareProgress& progress) = 0;
        virtual void OnStatusChange(const WinShareKit::HwShareStatus& status) = 0;
    };

}

class WIN_SHARE_KIT_SERVER_API SynergyServiceImpl
{
public:
    //搜索设备
    void SearchDevice(WinShareKit::DeviceChangeListener *deviceChangelistener);
    //停止搜索 TODO
    void StopSearch();

    //是否支持当前设备
    bool IsSupportDevice();
    //发送文件
    void SendByHwShare(WinShareKit::HwShareDevice& device, vector<std::string> vecSendFiles, WinShareKit::SendProgressListener *progressListener);
    //取消发送
    void CancelSend();

    //protected:
    SynergyServiceImpl(int clientId);
    ~SynergyServiceImpl();
};

class WIN_SHARE_KIT_SERVER_API ShareKitClient
{
public:
    //拉起ShareKitServer进程
    int StartShareKitServer();
    //客户端注册，获取IPCModuleID
    int RegistClient();
    //客户端注销
    int UnRegistClient();
};

typedef enum INIT_RESULT
{
    INIT_SUCCESS = 0,
    INIT_ERROR_CAN_NOT_GET_MUTEX = -10,
    INIT_ERROR_OTHERS = -1
}INIT_RESULT;

typedef enum UNINIT_RESULT
{
    UNINIT_SUCCESS = 0,
    UNINIT_ERROR_OTHERS = -1
}UNINIT_RESULT;

typedef enum START_RESULT
{
    START_SUCCESS = 0,
    START_ERROR_OTHERS = -1
}START_RESULT;

typedef enum STOP_RESULT
{
    STOP_SUCCESS = 0,
    STOP_ERROR_OTHERS = -1
}STOP_RESULT;

typedef enum STARTSERVER_RESULT
{
    START_SERVER_SUCCESS = 0,
    SERVER_ALREADY_EXIST = -10,
    START_SERVER_ERROR_OTHERS = -1
}STARTSERVER_RESULT;