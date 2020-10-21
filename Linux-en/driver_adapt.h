/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Driver interface definition
 * Create: 2019-05-17
 */

#ifndef DRIVER_ADAPT_H
#define DRIVER_ADAPT_H

#define MAC_ADDR_LEN 6      // device MAC address length
#define BLE_ADV_MAX_LEN 31  // ble advertise data max len
#define BLE_DRIVER_NAME "ble" // type of drive 

// define the function parameter types of GetParams
typedef enum {
    GET_DEV_MAC, 
    GET_MODEL_ID,
    GET_DEV_NAME,
} GetParamsType;

// define the function parameter types of SetParams
typedef enum {
    SET_PARAMS,
} SetParamsType;

typedef struct {
    /* ble gatt server */
    const char* serviceUuid;
    const char* readUuid;
    const char* writeUuid; 
    int serviceEnable;
    int attrPerm;   // Attribute permissions
	int charProp;  // characteristic properties

    /* ble gatt client */
    int minInterval;
    int maxInterval;
    int timeout;
    char* nearbyAdvData;
    int advDataLen;
} BleParams;

typedef struct {
    char modelId[3]; // Unique identification of device information
    char subModelId; // Device Sub Model ID
    int subModelFlag;// Identify if the device has a Sub-Model ID
} DeviceModelId;

typedef enum {
    EVENT_CONNECT = 0,  // CONNECT
    EVENT_CONNECT_FAIL, // CONNECT FAIL
    EVENT_DISCONNECT,   // DISCONNECT
} EventType;

typedef enum {
    CHANNEL_UNSET = 0, 
    CHANNEL_BLE = 1,
    CHANNEL_BREDR = 2, 
    CHANNEL_P2P = 3,
    CHANNEL_USB = 4,
} ChannelType;

#define BLE_ADV_INTERVAL_DEFAULT 0x0040
#define BLE_ADV_CHANNEL_37 1
#define BLE_ADV_CHANNEL_38 2
#define BLE_ADV_CHANNEL_39 4
#define BLE_ADV_CHANNEL_ALL (BLE_ADV_CHANNEL_37 | BLE_ADV_CHANNEL_38 | BLE_ADV_CHANNEL_39)

typedef struct {
    unsigned short minInterval;
    unsigned short maxInterval;
    unsigned char advChannel;
} BleAdvParams;

typedef struct {
    unsigned char advDataLen;
    unsigned char advData[BLE_ADV_MAX_LEN];
    unsigned char rspDataLen;
    unsigned char rspData[BLE_ADV_MAX_LEN];
} BleAdvData;

typedef enum {
    ADV_IND = 0x00,         // Connectable and scannable undirected advertising
    ADV_DIRECT_IND = 0x01,  // Connectable directed advertising
    ADV_SCAN_IND = 0x02,    // Scannable undirected advertising
    ADV_NONCONN_IND = 0x03, // Non connectable undirected advertising
    SCAN_RSP = 0x04,          // Scan Response
} BleScanEventType;

/**
 * @ingroup AllConnectSDK
 * @brief scan result struct
 * @brief for more details please check <<Bluetooth Core Specification>> 7.7.65.2 LE Advertising Report Event
 */
typedef struct {
    unsigned char evtType;  // BLE Advertising Report event ADV_IND ADV_SCAN_IND
    unsigned char addrType; // Device Address type Public or random
    unsigned char addr[MAC_ADDR_LEN];    // Device Address
    unsigned char len;     // Length of the data field max 31
    unsigned char data[BLE_ADV_MAX_LEN]; // Advertising or scan response data
    char rssi;  //  Range: -127 ≤ N ≤ +20 Units: dBm
} BleScanResult;

typedef struct {
    unsigned char addrType;  // Device Address type Public or random
    unsigned char addr[MAC_ADDR_LEN];  // Device Address
} BleConnectParam;

// Channel callback function
typedef struct {
    void (*ChannelStateChange)(ChannelType channelType, EventType evt, void* data, int dataLen);
    void (*ScanCallback)(ChannelType channelType, BleScanResult* result);
    void (*ReceiveData)(ChannelType channelType, void* channelParam, unsigned char* data, int dataLen);
} ChannelCallback;

// Define different types of devices (BLE, BR, Wi-Fi, etc.) abstract and normalized API interface, each device needs to be adapted
struct FcDriverAdaptOps {
    const char* name;
    const char* desc;

	/*
    * Initialization function
    * Param: *priv  the structure pointer of the data
    * Return: 0 on success, -1 on failure
    */
    int (*Init)(void* ctx, const char* ifName);

	/*
    * Uninitialization function to release resources
    * Param: *priv  the structure pointer of the data
    * Return: 0 on success, -1 on failure
    */
	int (*Deinit)(void* priv);

    /*
     * Set Drive parameter after call function of Init
     * Param: *priv  the structure pointer of the data
     * Return: 0 on success, -1 on failure
     */
	int (*SetParam)(SetParamsType paramType, void* priv);

    /*
     * Drive parameter acquisition function
     * Return: 0 on success, others on failure
     */
	int (*GetParam)(GetParamsType paramType, void* result);

    /*
     * Start BLE advertise
     * Return: 0 on success, others on failure
     */
	int (*StartAdv)(void* priv);

    /*
     * Stop BLE advertise
     * Return: 0 on success, others on failure
     */
    int (*StopAdv)(void* priv);

    /*
     * Configure content of BLE advertise
     * Return: 0 on success, others on failure
     */
    int (*SetAdvData)(void* priv);
    /*
     * Start BLE Scan
     * Return: 0 on success, others on failure
     */
    int (*StartScan)(void* priv);
    /*
     * Stop BLE Scan
     * Return: 0 on success, others on failure
     */
    int (*StopScan)(void* priv);

    /**
     * Start send data 
     * Return: 0 on success, others on failure
     */
    int (*SendData)(unsigned char* data, int dataLen, void* priv);

    /**
     * Server to send data to the client
     * Return: 0 on success, others on failure
     */
    int (*NotifyData)(unsigned char* data, int dataLen, void* priv);

    /*
     * Create Wifi Channel:P2P/AP
     * Returns:0 on success, others on failure
     */
	int (*CreateWifiChannel)(void* priv, void* wifiData);

    /*
     * Connect Channel
     * Returns: 0 on success, -1 on failure
     */
    int (*Connect)(void* priv);

    /*
     * Disconnect Channel
     * Returns: 0 on success, -1 on failure
     */
    int (*Disconnect)(void* priv);

    /*
     * Register Channel Callback
     * Returns: 0 on success, -1 on failure
     */
    int (*RegisterChannelCallback)(ChannelCallback* channelCallback);
};

#endif /* DRIVER_ADAPT_H */