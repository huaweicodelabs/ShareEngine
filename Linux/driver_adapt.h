/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Driver interface definition
 * Create: 2019-05-17
 */

#ifndef DRIVER_ADAPT_H
#define DRIVER_ADAPT_H

#define MAC_ADDR_LEN 6     /**< 设备MAC地址长度 */
#define BLE_ADV_MAX_LEN 31 /**< ble advertise data max len */
/**
 * @brief 定义驱动类型
 */
#define BLE_DRIVER_NAME "ble"

/**
 * @brief 定义GetParams操作获取设备相关参数类型
 */
typedef enum {
    GET_DEV_MAC,  /**< 获取设备的MAC地址 */
    GET_MODEL_ID, /**< 获取设备对应的设备识别码 */
    GET_DEV_NAME,
} GetParamsType;

/**
 * @brief 定义SetParams操作修改设备相关参数类型
 */
typedef enum {
    SET_PARAMS,
} SetParamsType;

/**
 * @brief 定义NEARBY中需要设置的BLE GATT各参数
 */
typedef struct {
    /* ble gatt server */
    const char* serviceUuid; /**< 使用Gatts进行Nearby数据通信时，需要的service uuid */
    const char* readUuid;    /**< Gatt service对应的read characteristic uuid */
    const char* writeUuid;   /**< Gatt service对应的write characteristic uuid */
    int serviceEnable;       /**< Gatts 是否启动service，0:启动；1:启动 */
    int attrPerm;            /**< GATT协议标准中属性(Attribute)读写权限(permissions) */
    int charProp;            /**< GATT协议标准中特征(characteristic)对应的性质(properties) */
    /* ble gatt client */
    int minInterval;     /**< Gatt client发广播时最小时间间隔 */
    int maxInterval;     /**< Gatt client发广播时最大时间间隔 */
    int timeout;         /**< Gatt client广播超时时间。0表示一直发广播 */
    char* nearbyAdvData; /**< Gatt client发送广播数据 */
    int advDataLen;      /**< Gatt client发送广播数据长度 */
} BleParams;

/**
 * @brief GetParamsType中各类型对应返回数据的结构的定义
 */
typedef struct {
    char modelId[3]; /**< 设备型号ID, 向华为申请的用于查询设备信息的唯一标识, Nearby协议规定长度3个字节 */
    char subModelId;  /**< 设备子型号ID，用于区分不同外形特征的同款设备 */
    int subModelFlag; /**< 标识设备是否有Sub-Model ID，1表示有submodel ID，0表示没有submodel ID */
} DeviceModelId;

/**
 * @ingroup AllConnectSDK
 * @brief Event type
 */
typedef enum {
    EVENT_CONNECT = 0,  /**< CONNECT */
    EVENT_CONNECT_FAIL, /**< CONNECT FAIL*/
    EVENT_DISCONNECT,   /**< DISCONNECT */
} EventType;

/**
 * @ingroup AllConnectSDK
 * @brief 通道类型
 */
typedef enum {
    CHANNEL_UNSET = 0, /**< 通道 */
    CHANNEL_BLE = 1,   /**< 蓝牙BLE通道 */
    CHANNEL_BREDR = 2, /**< 蓝牙BR通道 */
    CHANNEL_P2P = 3,   /**< WIFI P2P通道 */
    CHANNEL_USB = 4,   /**< USB通道 */
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
    ADV_IND = 0x00,         /**< Connectable and scannable undirected advertising */
    ADV_DIRECT_IND = 0x01,  /**< Connectable directed advertising */
    ADV_SCAN_IND = 0x02,    /**< Scannable undirected advertising */
    ADV_NONCONN_IND = 0x03, /**< Non connectable undirected advertising */
    SCAN_RSP = 0x04,        /**< Scan Response */
} BleScanEventType;

/**
 * @ingroup AllConnectSDK
 * @brief scan result struct
 * @brief for more details please check <<Bluetooth Core Specification>> 7.7.65.2 LE Advertising Report Event
 */
typedef struct {
    unsigned char evtType;               /**< LE Advertising Report event ADV_IND ADV_SCAN_IND */
    unsigned char addrType;              /**< Device Address type Public or random */
    unsigned char addr[MAC_ADDR_LEN];    /**< Device Address */
    unsigned char len;                   /**< Length of the data field max 31 */
    unsigned char data[BLE_ADV_MAX_LEN]; /**< Advertising or scan response data */
    char rssi;                           /**< Range: -127 ≤ N ≤ +20 Units: dBm */
} BleScanResult;

typedef struct {
    unsigned char addrType;           /**< Device Address type Public or random */
    unsigned char addr[MAC_ADDR_LEN]; /**< Device Address */
} BleConnectParam;

/**
 * @ingroup AllConnectSDK
 * @brief 通道回调函数
 */
typedef struct {
    void (*ChannelStateChange)(ChannelType channelType, EventType evt, void* data, int dataLen);
    void (*ScanCallback)(ChannelType channelType, BleScanResult* result);
    void (*ReceiveData)(ChannelType channelType, void* channelParam, unsigned char* data, int dataLen);
} ChannelCallback;
/**
 * struct FcDriverOps - Driver interface API definition
 * 定义各不同类型设备(BLE、BR、Wi-Fi等)抽象归一API接口，各设备需要进行适配
 */
struct FcDriverAdaptOps {
    /* 设备接口名称 */
    const char* name;
    /* 设备接口简要描述，比如：mtk ble */
    const char* desc;

    /**
     * 初始化函数
     * 确保函数执行完成以后，其他驱动接口可用
     * 返回值: 0 on success, -1 on failure
     */
    int (*Init)(void* ctx, const char* ifName);
    /**
     * 去初始化函数
     * 函数执行完成以后，释放资源
     * 返回值: 0 on success, -1 on failure
     */
    int (*Deinit)(void* priv);
    /*
     * 驱动参数设置函数
     * 在Init 执行完成之后，将配置参数进行一次性传递
     * 参数: *priv 设置数据的结构体指针， 采用void *通配不同设备设置数据结构体差异
     * 返回值: 0 on success, -1 on failure
     * 可以消息通知完成全部参数设置
     */
    int (*SetParam)(SetParamsType paramType, void* priv);
    /*
     * 驱动参数获取函数
     * 参数: const char *param 获取参数类型，
     * 采用void *通配不同设备返回的类型结果差异,result 保存输出结果
     * 返回值: 0 on success, others on failure
     */
    int (*GetParam)(GetParamsType paramType, void* result);
    /*
     * 开启广播
     * 参数: *priv 数据的结构体指针，采用void *通配不同设备数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*StartAdv)(void* priv);
    /*
     * 关闭广播
     * 参数: *priv 数据的结构体指针，采用void *通配不同设备数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*StopAdv)(void* priv);

    /*
     * 配置广播内容
     * 参数: *priv 数据的结构体指针，采用void *通配不同设备数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*SetAdvData)(void* priv);
    /*
     * 开启广播扫描
     * 参数: *priv 数据的结构体指针，采用void *通配不同设备数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*StartScan)(void* priv);
    /*
     * 关闭广播扫描
     * 参数: *priv 数据的结构体指针，采用void *通配不同设备数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*StopScan)(void* priv);

    /**
     * 发送数据
     * 参数: data是待发送的数据，dataLen是待发送数据的长度
     *       *priv 发送数据的结构体指针，采用void *通配不同设备发送数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*SendData)(unsigned char* data, int dataLen, void* priv);

    /**
     * 用于服务器向客户端发送数据
     * 参数: data是待发送的数据，dataLen是待发送数据的长度
     *       *priv 发送数据的结构体指针，采用void *通配不同设备发送数据结构体差异
     * 返回值: 0 on success, others on failure
     */
    int (*NotifyData)(unsigned char* data, int dataLen, void* priv);

    /*
     * DisplayPin - 显示或播报PIN接口
     * 参数priv: 显示操作所需数据，通配各设备参数差异
     * 参数pin: 待显示PIN
     * Returns 0 on success,  others on failure
     */
    int (*DisplayPin)(void* priv, const unsigned char* pin);
    /*
     * PustMsgBox - 提供消息提示框接口
     * 参数priv: 通配各设备参数差异
     * Returns 0 on success, others on failure
     */
    int (*PushMsgBox)(void* priv);
    /*
     * CreateWifiChannel - 创建WIFI通道
     * 参数priv: 显示操作所需数据, 通配各设备参数差异; wifiDta: wifi具体信息
     * Returns 0 on success, others on failure
     */
    int (*CreateWifiChannel)(void* priv, void* wifiData);
    /*
     * 建立连接通道
     * 参数: 参数priv: 通配各设备参数差异
     * 返回值: 0 on success, -1 on failure
     */
    int (*Connect)(void* priv);

    /*
     * 断开连接通道
     * 参数: 参数priv: 通配各设备参数差异
     * 返回值: 0 on success, -1 on failure
     */
    int (*Disconnect)(void* priv);

    /*
     * RegisterChannelCallback
     * 参数: *ChannelCallback 通道连接回调
     * 返回值: 0 on success, -1 on failure
     */
    int (*RegisterChannelCallback)(ChannelCallback* channelCallback);
};

#endif /* DRIVER_ADAPT_H */