#include "locale.h"
#include "MyShareListener.h"
#include "Synergy/SynergyServiceImpl.h"


extern WinShareKit::HwShareDevice g_sharedevice;
extern int isDeviceFound;

std::wstring UTF8ToUnicode(const std::string& str)
{
    std::wstring  rt;
    if (str.length() > 0) {
        int unicodeLen = ::MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            -1,
            NULL,
            0);
        wchar_t * pUnicode = new wchar_t[unicodeLen + 1];
        SecureZeroMemory(pUnicode, (unicodeLen + 1) * sizeof(wchar_t));
        ::MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            -1,
            (LPWSTR)pUnicode,
            unicodeLen);
        rt.assign(pUnicode);
        delete[] pUnicode;
    }
    return  rt;
}

MyDeviceChangeListener::MyDeviceChangeListener()
{
}


MyDeviceChangeListener::~MyDeviceChangeListener()
{
}

void MyDeviceChangeListener::OnDeviceFound(const WinShareKit::HwShareDevice& device) {
    std::wstring devName = UTF8ToUnicode(device.DevName);
    setlocale(LC_CTYPE, "chs");
    wprintf(L"[callback]onDeviceFound,deviceName:%ws \n", devName.c_str());
    if (device.DevName.find("biyantas") != std::string::npos) {
        wprintf(L"my device was found,g_sharedevice is %ws ,shareType:%d", devName.c_str(), device.shareType);
        g_sharedevice = device;
        isDeviceFound = 1;
    }
}
void MyDeviceChangeListener::OnDeviceLost(const WinShareKit::HwShareDevice& device) {
    std::wstring devName = UTF8ToUnicode(device.DevName);
    setlocale(LC_CTYPE, "chs");
    wprintf(L"[callback]OnDeviceLost,deviceName:%ws \n", devName.c_str());
}

MySendProgressListener::MySendProgressListener()
{
}


MySendProgressListener::~MySendProgressListener()
{
}

void MySendProgressListener::OnProgress(const WinShareKit::HwShareProgress& progress)
{
    printf("MySendProgressListener::OnProgress:%d", progress.progress);
}

extern int isSendFinish;
void MySendProgressListener::OnStatusChange(const WinShareKit::HwShareStatus& status)
{
    printf("MySendProgressListener::OnProgress:%d", status.intStatus);
    switch (status.intStatus)
    {
    case WinShareKit::DTCP_STATUS_SEND_CONNECTING:
        printf("正在连接...");
        return;

    case WinShareKit::DTCP_STATUS_SEND_CONNECTED:
        printf("连接成功");
        return;

    case WinShareKit::DTCP_STATUS_SEND_PREVIEW:
        printf("开始发送预览数据");
        return;

    case WinShareKit::DTCP_STATUS_SEND_PREVIEWOK:
        printf("预览数据发送完成");
        return;

    case WinShareKit::DTCP_STATUS_SEND_DATA:
        printf("开始发送文件数据");
        return;

    case WinShareKit::DTCP_STATUS_SEND_DATAOK:
        printf("文件数据发送完成");
        return;

    case WinShareKit::DTCP_STATUS_SEND_RECVCANCEL:
        printf("接收方取消");
        break;

    case WinShareKit::DTCP_STATUS_SEND_RECVREJCT:
        printf("接收方拒绝");
        break;

    case WinShareKit::DTCP_ERR_NOSPACE:
        printf("接收方空间不足");
        break;

    case WinShareKit::SHARE_ERROR_SERVER_BUSY:
        printf("正在接收");
        break;

    case WinShareKit::SHARE_ERROR_CLIENT_BUSY:
    case WinShareKit::SHARE_ERROR_SELF_CHANNEL_BUSY:
        printf("本机忙");
        break;

        //case synergy::SHARE_ERROR_CHANNEL_BUSY:
    case WinShareKit::SHARE_ERROR_OTHER_CHANNEL_BUSY:
        printf("对方正忙");
        break;

    case WinShareKit::DTCP_STATUS_SEND_CANCEL:
        printf("取消发送成功");
        break;

    case WinShareKit::SHARE_ERROR_SOCKET:
    case WinShareKit::DTCP_ERR_SOCKET:
        printf("Socket出错，传输失败");
        //if (s_bConnected)
        //{
        //	printf(_T("Socket出错，传输失败"), true);
        //}
        //else
        //{
        //	printf(_T("连接失败"), true);
        //}
        break;

    case WinShareKit::SHARE_ERROR_REMOTE:
    case WinShareKit::DTCP_ERR_REMOTE:
        printf("接收端出错");
        break;

    case WinShareKit::SHARE_ERROR_IO:
    case WinShareKit::DTCP_ERR_IO:
        printf("I/O错误");
        break;

    case WinShareKit::SHARE_UNKNOWN_ERROR:
        printf("未知错误");
        break;

    case WinShareKit::DTCP_TIMEOUT_FINISH:
        printf("接收方确认文件数据接收超时");
        break;

    case WinShareKit::DTCP_TIMEOUT_CONFIRM:
        printf("接收方确认接收超时");
        break;

    case WinShareKit::DTCP_TIMEOUT_CANCEL:
        printf("接收方响应取消超时");
        break;

    case WinShareKit::DTCP_STATUS_SEND_SUCCESS:
        printf("文件分享成功");
        break;

    case WinShareKit::DTCP_STATUS_SEND_DISCONNECTED:
        printf("连接关闭");
		isSendFinish = 1;
        break;

    case WinShareKit::DTCP_WIFI_EXCEPTION_DISCONNECT:
        printf("PC为发送端，WiFi异常断开");
        break;

    case WinShareKit::DTCP_BT_OR_WIFI_CLOSE:
        printf("PC为发送端，PC侧断开WiFi");

    default:
        return;
    }
}