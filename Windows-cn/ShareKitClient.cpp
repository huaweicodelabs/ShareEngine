// ShareKitClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "ShareKitClient.h"
#include "MyShareListener.h"
#include <io.h>
#include <csignal>
#include <memory>

#pragma comment (lib, "ShareService.lib")

MyDeviceChangeListener *deviceListener = new MyDeviceChangeListener();
MySendProgressListener *progressListener = new MySendProgressListener();


static volatile int keepRunning = 1;


ShareKitClient ShareClient;

void sig_handler(int sig)
{
    if (sig == SIGINT || sig == SIGBREAK)
    {
        printf("close \n");
        ShareClient.UnRegistClient();
        Sleep(500);
    }
}

void getAllFiles(string path, vector<string>& files, string format)
{
    long long hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*" + format).c_str(), &fileinfo)) != -1)
    {
        do
        {
            if ((fileinfo.attrib & _A_SUBDIR))
            {

            }
            else
            {
                files.push_back(p.assign(path).append("\\").append(fileinfo.name));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

std::string AnsiToUtf8(std::string src)
{
    // convert an ansi string to widechar
    std::string u8;
    int32_t nLen = MultiByteToWideChar(CP_ACP, 0, src.c_str(), src.length(), NULL, 0);

    //WCHAR* lpszW = NULL;

    auto lpszW = std::make_unique<WCHAR[]>(nLen);

    int32_t nRtn = MultiByteToWideChar(CP_ACP, 0, src.c_str(), src.length(), lpszW.get(), nLen);

    if (nRtn != nLen)
    {
        //delete[] lpszW;
        return "";
    }

    // convert an widechar string to utf8s
    int32_t MBLen = WideCharToMultiByte(CP_UTF8, 0, lpszW.get(), nLen, NULL, 0, NULL, NULL);
    if (MBLen <= 0)
    {
        return "";
    }
    u8.resize(MBLen);
    nRtn = WideCharToMultiByte(CP_UTF8, 0, lpszW.get(), nLen, &u8.front(), MBLen, NULL, NULL);
    //delete[] lpszW;

    if (nRtn != MBLen)
    {
        u8.clear();
        return "";
    }
    return u8;
}

// SynergySerObj此处使用引用，保证对象不被析构
int sendFile(SynergyServiceImpl &SynergySerObj)
{
    printf("begin sending \n");
    //printf("SynergySer:%p", &SynergySer);
    SynergySerObj.SearchDevice((WinShareKit::DeviceChangeListener *)deviceListener);
    while (!isDeviceFound && true) {
        Sleep(2000);
    }
    vector<std::string> vecSendFiles;

    const char* filePath = "D:\\share";
    std::string m_format = "";
    vector<string> files;
    getAllFiles(filePath, files, m_format);
    for (auto it = files.begin(); it != files.end(); ++it) {
        string tmp1 = *it;
        std::string utf8str = AnsiToUtf8(tmp1);
        vecSendFiles.push_back(utf8str);
    }
    SynergySerObj.SendByHwShare(g_sharedevice, vecSendFiles, (WinShareKit::SendProgressListener *)progressListener);

    while (!isSendFinish) {
        Sleep(2000);
    }
    SynergySerObj.StopSearch();
    isSendFinish = 0;
    printf("end sending \n");
    return 0;
}

int UnRegistAndExit()
{
    ShareClient.UnRegistClient();
    exit(0);
}

int main()
{
    signal(SIGINT, sig_handler);
    signal(SIGBREAK, sig_handler);

    ShareClient.StartShareKitServer();
    int clientId = ShareClient.RegistClient();
    // SynergyServiceImpl此对象必须保证在整个功能的生命周期，保证在注销时此对象还存活，否则会导致ShareKitServer无法退出，功能异常
    SynergyServiceImpl SynergySerObj(clientId);

    int i = 2;
    bool success = false;

    while (keepRunning) {
        std::cout << "1:sendFile，2:UnRegistAndExit " << std::endl;
        std::cin >> i;
        if (i == 1)
        {
            success = sendFile(SynergySerObj) == 0;
        }
        else if (i == 2)
        {
            success = UnRegistAndExit() == 0;
        }
        else
        {
            printf("other option \n");
        }
    }


    ShareClient.UnRegistClient();

    Sleep(2000);
    return 0;
}
