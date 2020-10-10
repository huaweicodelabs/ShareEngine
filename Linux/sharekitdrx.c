#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <dirent.h>
#include "sharekit_api.h"

static int g_running = 1;

static void Exit()
{
    printf("Exit \n");
    g_running = 0;
}

void SharekitStatusChangeCbFunc(ShareStatus* status)
{
    if (status->statusType == BLE_TYPE) {
        if (strlen(status->statusInfo) != 0) {
            printf("Error info : %s\n", status->statusInfo);
        }
        Exit();
    }

	if (status->statusType == WIFI_TYPE) {
        Exit();
    }
    return;
}

void SharekitProcessCbFunc(int process)
{
    printf("file transfer process : %d\n", process);

    return;
}

void RequestFileTrans(const char** fileNames, int fileNum)
{
#define FILE_PATH "./"
    printf("=====> request to trans file, fileNum %d, please insert y/n <=====\n", fileNum);
    if (fileNum > 0) {
        for (int i = 0; i < fileNum; i++) {
            printf("=====>file name %s<=====\n", fileNames[i]);
        }
    }
    ConfirmRecvFile(FILE_PATH);
}

void FileTransferResult(int result, const char* failedFileNames[], int failedFileNum)
{
    printf("=====> file transfer finish, try to release resource <=====\n");
    Exit();
}

int main(int argc, char* argv[])
{
    signal(SIGINT, Exit);

    if (InitShareKit(NULL) != 0) {
        goto done;
    }

    ShareRecvHandle recvHandler;
    memset(&recvHandler, 0, sizeof(ShareRecvHandle));
	recvHandler.progressCb = SharekitProcessCbFunc;
    recvHandler.statusCb = SharekitStatusChangeCbFunc;
    recvHandler.previewCb = RequestFileTrans;
    recvHandler.finishCb = FileTransferResult;

    if (StartShareService(RECEIVER, &recvHandler) != 0) {
        goto done;
    }

    while (g_running) {
        usleep(50000);
    }

done:
    StopShareService();
    UninitShareKit();
    sleep(1);
    printf("main exit. \n");
    return 0;
}
