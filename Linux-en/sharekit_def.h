/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: AllConnect Platform ShareEngine API common define
 * Create: 2019-10-10
 */

#ifndef SHAREKIT_DEF_H
#define SHAREKIT_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAC_ADDR_LEN 6
#define DEVICE_NAME_LEN 26
#define DEVICE_SUMMARY_LEN 20
#define SHARE_STATUS_INFO_LEN 128
#define SHAREKIT_DEVICE_VERSION "sharekit-device-linux-1.0.0.100T1"

// Sharing status type
typedef enum {
    BASE_TYPE = 0,  // Base type
    BLE_TYPE = 1,   // Ble type
    WIFI_TYPE = 2,  // Wifi type
} ShareStatusType;

// Sharing status info
typedef struct {
    ShareStatusType statusType;              // Sharing status type
    char statusInfo[SHARE_STATUS_INFO_LEN];  // Sharing status info
} ShareStatus;

// Type of the shared device
typedef enum {
    DEVICE_TYPE_PHONE = 0,   // phone
    DEVICE_TYPE_PHONE1 = 1,  // phone
    DEVICE_TYPE_PC = 2,      // PC
} ShareDevType;

// Information of the shared device
typedef struct {
    ShareDevType type;                          // device type
    char rssi;                                  // device rssi
    char addrType;                              // device mac type, random or public
    char summaryLen;                            // device summary len, 0 means no summary
    char name[DEVICE_NAME_LEN];                 // device name, usually bt name or huawei id
    unsigned char addr[MAC_ADDR_LEN];           // device mac addr, use to bt connect
    unsigned char summary[DEVICE_SUMMARY_LEN];  // device summary, usually save hashcode
} ShareDeviceInfo;

// Information of the shared file
typedef struct {
    int fileNum;     // Number of the shared file
    char** uriList;  //  URI List of the shared file
} ShareFileInfo;

// result of the shared file
typedef enum {
    SHARE_SUCCESS = 0,        // Share all sucess
    SHARE_FAILED = 1,         // Share faild
    SHARE_PART_SUCCESS = 2,  // Share Partial success
} ShareResult;

/*
 * Share SDK progress callback function
 * SDK notifies the progress to the user application
 * @param process: Sharing progress
 */
typedef void (*ShareProgressCallback)(int process);
/*
 * Share SDK status callback function
 * SDK notifies the status to the user application
 * @param status: Sharing status info
 */
typedef void (*ShareStatusCallback)(ShareStatus* status);

/*
 * Share SDK file preview information callback function
 * SDK notifies the file preview information to the user application
 * @param fileNames: List of sharing file name
 * @param fileNum: Number of sharing file
 */
typedef void (*ShareFilePreviewCallback)(const char** fileNames, int fileNum);

/*
 * Share SDK file share finish callback function
 * SDK notifies the result to the user application
 * @param result: share result(enum value of ShareResult)
 * @param failedNames: List of sharing failed file name
 * @param fileNum: Number of sharing failed file
 */
typedef void (*ShareFileFinishCallback)(int result, const char** failedFileNames, int failedFileNum);

// Input parameters of start transmits
typedef struct {
    ShareProgressCallback progressCb;  // Callback function of sharing progress
    ShareStatusCallback statusCb;      // Callback function of sharing status
} ShareTransHandle;

// Input parameters of start receive
typedef struct {
    ShareProgressCallback progressCb;  // Callback function of sharing progress
    ShareStatusCallback statusCb;      // Callback function of sharing status

    ShareFilePreviewCallback previewCb;  // Callback function of file preview
    ShareFileFinishCallback finishCb;    // Callback function of file share finish
} ShareRecvHandle;

// Roles of the share app
typedef enum {
    RECEIVER = 1,     // Receiver role
    TRANSMITTER = 2,  // Transmitter role
} ShareRole;

// Error code of the share kit
typedef enum {
    ERROR_COMMON_BASE = 1,  // Common error code start
    ERROR_REGISTERCALLBACK_FAIL,

    ERROR_BT_BASE = 1000,  // Bluetooth error code start
    ERROR_BT_INIT_FAIL,
    ERROR_BT_START_SCAN_FAIL,
    ERROR_BT_START_ADV_FAIL,
    ERROR_BT_CONNECT_FAIL,
    ERROR_BT_SEND_DATA_FAIL,

    ERROR_WIFI_BASE = 2000,  // Wifi error code start
    ERROR_WIFI_CONNECT_FAIL,
} ShareErrorCode;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SHAREKIT_DEF_H */
