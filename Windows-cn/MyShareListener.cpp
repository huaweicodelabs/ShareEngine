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
        printf("��������...");
        return;

    case WinShareKit::DTCP_STATUS_SEND_CONNECTED:
        printf("���ӳɹ�");
        return;

    case WinShareKit::DTCP_STATUS_SEND_PREVIEW:
        printf("��ʼ����Ԥ������");
        return;

    case WinShareKit::DTCP_STATUS_SEND_PREVIEWOK:
        printf("Ԥ�����ݷ������");
        return;

    case WinShareKit::DTCP_STATUS_SEND_DATA:
        printf("��ʼ�����ļ�����");
        return;

    case WinShareKit::DTCP_STATUS_SEND_DATAOK:
        printf("�ļ����ݷ������");
        return;

    case WinShareKit::DTCP_STATUS_SEND_RECVCANCEL:
        printf("���շ�ȡ��");
        break;

    case WinShareKit::DTCP_STATUS_SEND_RECVREJCT:
        printf("���շ��ܾ�");
        break;

    case WinShareKit::DTCP_ERR_NOSPACE:
        printf("���շ��ռ䲻��");
        break;

    case WinShareKit::SHARE_ERROR_SERVER_BUSY:
        printf("���ڽ���");
        break;

    case WinShareKit::SHARE_ERROR_CLIENT_BUSY:
    case WinShareKit::SHARE_ERROR_SELF_CHANNEL_BUSY:
        printf("����æ");
        break;

        //case synergy::SHARE_ERROR_CHANNEL_BUSY:
    case WinShareKit::SHARE_ERROR_OTHER_CHANNEL_BUSY:
        printf("�Է���æ");
        break;

    case WinShareKit::DTCP_STATUS_SEND_CANCEL:
        printf("ȡ�����ͳɹ�");
        break;

    case WinShareKit::SHARE_ERROR_SOCKET:
    case WinShareKit::DTCP_ERR_SOCKET:
        printf("Socket��������ʧ��");
        //if (s_bConnected)
        //{
        //	printf(_T("Socket��������ʧ��"), true);
        //}
        //else
        //{
        //	printf(_T("����ʧ��"), true);
        //}
        break;

    case WinShareKit::SHARE_ERROR_REMOTE:
    case WinShareKit::DTCP_ERR_REMOTE:
        printf("���ն˳���");
        break;

    case WinShareKit::SHARE_ERROR_IO:
    case WinShareKit::DTCP_ERR_IO:
        printf("I/O����");
        break;

    case WinShareKit::SHARE_UNKNOWN_ERROR:
        printf("δ֪����");
        break;

    case WinShareKit::DTCP_TIMEOUT_FINISH:
        printf("���շ�ȷ���ļ����ݽ��ճ�ʱ");
        break;

    case WinShareKit::DTCP_TIMEOUT_CONFIRM:
        printf("���շ�ȷ�Ͻ��ճ�ʱ");
        break;

    case WinShareKit::DTCP_TIMEOUT_CANCEL:
        printf("���շ���Ӧȡ����ʱ");
        break;

    case WinShareKit::DTCP_STATUS_SEND_SUCCESS:
        printf("�ļ�����ɹ�");
        break;

    case WinShareKit::DTCP_STATUS_SEND_DISCONNECTED:
        printf("���ӹر�");
		isSendFinish = 1;
        break;

    case WinShareKit::DTCP_WIFI_EXCEPTION_DISCONNECT:
        printf("PCΪ���Ͷˣ�WiFi�쳣�Ͽ�");
        break;

    case WinShareKit::DTCP_BT_OR_WIFI_CLOSE:
        printf("PCΪ���Ͷˣ�PC��Ͽ�WiFi");

    default:
        return;
    }
}