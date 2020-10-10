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

#define SEND_PATH "./"
#define SHOW_MAX_NUM 10
#define MAX_PATH_LEN 64

static int g_running = 1;

static void Exit()
{
    printf("Exit \n");
    g_running = 0;
}

void ShowShareKitDeviceList(ShareDeviceInfo* list, int devNum)
{
    if ((list == NULL) || (devNum <= 0)) {
        printf("\033c");
        printf("-----------------START-----------------------\n");
        printf("list is NULL\n");
        printf("------------------END------------------------\n");
        return;
    }

    printf("\033c");  // clean screen.
    printf("-----------------START-----------------------\n");
    printf("No.      Mac addr  \t Rssi\t Name\n");

    int ind;
    for (ind = 0; ind < devNum; ind++) {
        printf(" %d ", ind + 1);
        printf(" %02x:%02x:%02x:%02x:%02x:%02x\t",
            (unsigned char)list[ind].addr[5],
            (unsigned char)list[ind].addr[4],
            (unsigned char)list[ind].addr[3],
            (unsigned char)list[ind].addr[2],
            (unsigned char)list[ind].addr[1],
            (unsigned char)list[ind].addr[0]);
        printf(" %d\t", list[ind].rssi);
        printf(" %s\t", list[ind].name);
        printf(" [%d]", list[ind].summaryLen);
        int i;
        for (i = 0; i < list[ind].summaryLen; i++) {
            printf(" %02x", list[ind].summary[i]);
        }
        printf("\n");
    }
    printf("------------------END------------------------\n");
    printf("Type Dev serial to select dev!\n");
    return;
}

// select a device from list and return a device.
ShareDeviceInfo* GetSelectedDev(int ind, ShareDeviceInfo* list, int devNum)
{
    if ((list == NULL) || (ind < 1) || (ind > devNum)) {
        return NULL;
    }

    ShareDeviceInfo* dev = NULL;
    dev = (ShareDeviceInfo*)malloc(sizeof(*dev));
    if (dev == NULL) {
        printf("Malloc failed.\n");
        return NULL;
    }
    memset(dev, 0, sizeof(*dev));
    memcpy(dev, &list[ind - 1], sizeof(list[ind - 1]));

    return dev;
}

int Kbhit(void)  // mornitor if typed in a num.
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int IsSelected(int* ch)  // check selected num is valid or not.
{
    if (Kbhit()) {
        *ch = getchar() - '0';
        if (*ch > 0 && *ch <= SHOW_MAX_NUM) {
            return *ch;
        } else {
            *ch = 0;
        }
    }
    return 0;
}

ShareDeviceInfo* SelectDev()
{
    int devNum;
    int selectNum = 0;
    int flushTime = 2;  // flush device list every 2s.
    ShareDeviceInfo* devList = NULL;
    ShareDeviceInfo* dev = NULL;  // save device which is selected.

    while (!IsSelected(&selectNum) && (g_running == 1)) {
        if (devList != NULL) {
            free(devList);
        }
        GetAvailableDevice(&devList, &devNum);
        ShowShareKitDeviceList(devList, devNum);
        sleep(flushTime);
    }

    dev = GetSelectedDev(selectNum, devList, devNum);
    if (devList != NULL) {
        free(devList);
    }

    return dev;
}

char** ScanFileName(char* path, int* fileNum);
char* GetMimeType(char* name);
int CountFileNum(char* path);
int CheckMimeType(char* myType, char** type);

static char* g_type[4] = {"png", "jpg", "bmw", "jpeg"};

char** ScanFileName(char* path, int* fileNum)
{
    if ((path == NULL) || (fileNum == NULL)) {
        return NULL;
    }

    *fileNum = CountFileNum(path);
    if (*fileNum == 0) {
        return NULL;
    }

    DIR* dp = NULL;
    struct dirent* dirp = NULL;
    if ((dp = opendir(path)) == NULL) {
        printf("can't open %s \n", path);
        return NULL;
    }

    char** namelist = NULL;
    namelist = (char**)malloc(sizeof(char*) * (*fileNum));
    memset(namelist, 0, sizeof(char*) * (*fileNum));

    int ind = 0;
    while ((dirp = readdir(dp)) != NULL) {
        char* type = GetMimeType(dirp->d_name);
        if (CheckMimeType(type, g_type)) {
            namelist[ind] = (char*)malloc(sizeof(char) * MAX_PATH_LEN);
            memset(namelist[ind], 0, sizeof(char) * MAX_PATH_LEN);
            strcat(namelist[ind], path);
            strcat(namelist[ind], "/");
            strcat(namelist[ind], dirp->d_name);
            ind++;
        }
    }
    closedir(dp);

    return namelist;
}

int CountFileNum(char* path)
{
    int cnt = 0;
    DIR* dp;
    struct dirent* dirp;

    if ((dp = opendir(path)) == NULL) {
        return 0;
    }

    while ((dirp = readdir(dp)) != NULL) {
        char* type = GetMimeType(dirp->d_name);
        if (CheckMimeType(type, g_type)) {
            cnt++;
        }
    }
    closedir(dp);

    return cnt;
}

int CheckMimeType(char* myType, char** type)
{
    if ((myType == NULL) || (type == NULL)) {
        return 0;
    }

    int ind = 0;
    while (ind < 4) {
        if (strcasecmp(myType, type[ind]) == 0) {
            return 1;
        } else {
            ind++;
        }
    }

    return 0;
}

char* GetMimeType(char* name)
{
    if (name == NULL) {
        return NULL;
    }

    int tLen = strlen(name);
    name += tLen - 1;
    int i = tLen - 1;
    for (; i > 0; i--) {
        if ((*name == '.') || (*name == '\\') || (*name == '/')) {
            name++;
            break;
        } else {
            name--;
        }
    }

    return name;
}

void FreeFileList(char** list, int fileNum)
{
    if (list == NULL) {
        return;
    }
    int i = 0;
    for (; i < fileNum; i++) {
        if (list[i] != NULL) {
            free(list[i]);
            list[i] = NULL;
        }
    }
    if (list != NULL) {
        free(list);
        list = NULL;
    }
    return;
}

void SharekitStatusChangeCbFunc(ShareStatus* status)
{
    if (status->statusType == BLE_TYPE) {
        if (strlen(status->statusInfo) != 0) {
            printf("info : %s\n", status->statusInfo);
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

int main(int argc, char* argv[])
{
    signal(SIGINT, Exit);
    int num = 0;
    printf("Debug tx for role %d.\n", TRANSMITTER);

    char** uriList = ScanFileName(SEND_PATH, &num);
    if (uriList == NULL || *uriList == NULL || num <= 0) {
        printf("scan file failed\n");
        return -1;
    }

    if (InitShareKit(NULL) != 0) {
        goto done;
    }


    ShareTransHandle transHandle;
    memset(&transHandle, 0, sizeof(ShareTransHandle));
	transHandle.progressCb = SharekitProcessCbFunc;
    transHandle.statusCb = SharekitStatusChangeCbFunc;

    if (StartShareService(TRANSMITTER, &transHandle) != 0) {
        goto done;
    }

    ShareDeviceInfo* dev = NULL;
    dev = SelectDev();

    ShareFileInfo files;
    memset(&files, 0, sizeof(ShareFileInfo));
    files.uriList = uriList;
    files.fileNum = num;
    if (dev != NULL) {
        printf(" %02x:%02x:%02x:%02x:%02x:%02x\n",
            dev->addr[5],
            dev->addr[4],
            dev->addr[3],
            dev->addr[2],
            dev->addr[1],
            dev->addr[0]);
        (void)TransFile(dev, &files);
    }

    while (g_running) {
        usleep(50000);
    }

done:
    FreeFileList(uriList, num);
    StopShareService();
    UninitShareKit();
    sleep(1);
    printf("main exit. \n");
    return 0;
}
