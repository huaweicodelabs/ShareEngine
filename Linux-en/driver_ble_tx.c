/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Driver interface definition
 * Create: 2019-05-29
 */

#include "driver_adapt.h"

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/l2cap.h"
#include "lib/uuid.h"

#include "src/shared/mainloop.h"
#include "src/shared/util.h"
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-client.h"

struct connect {
    bdaddr_t src;
    bdaddr_t dst;
    uint8_t dstType;
};

struct client {
    int fd;
    struct bt_att* att;
    struct gatt_db* db;
    struct bt_gatt_client* gatt;
    uint16_t g_nearby_handle;
    uint16_t g_notify_handle;
    uint16_t g_desc_handle;
};

#define HCI_DEV_ADDRTYPE LE_PUBLIC_ADDRESS
#define HCI_FILTER_DUP_FLAG 0x00

typedef struct {
    uint8_t evt_type;
    uint8_t bdaddr_type;
    bdaddr_t bdaddr;
    uint8_t length;
    uint8_t data[31];
    int8_t rssi;
} le_advertising_report;

int g_dev_id;         // hciX dev id
pthread_t g_scan_pt;  // scan thread id
int g_scan_running;   // scan thread runing status
ChannelCallback* g_channel_cb;

struct client* g_nearbyCli = NULL;

static int stop_advertise(int dd);
static int start_scan(int dd, uint8_t type, uint16_t interval, uint16_t window, uint8_t filter_dup);
static int stop_scan(int dd, uint8_t filter_dup);
static void start_get_result(void);
static void stop_get_result(void);
static int hci_le_set_advertise_parameters(
    int dd, uint16_t min_interval, uint16_t max_interval, uint8_t chan_map, int to);
static int hci_le_set_advertise_data(int dd, uint8_t length, uint8_t* data, int to);
static int hci_le_set_scan_response_data(int dd, uint8_t length, uint8_t* data, int to);

extern int BleSendResponse(unsigned char* value, int valLen);

static int BleDriverInit(void* ctx, const char* ifname)
{
    int g_dev_id = hci_get_route(NULL);
    if (g_dev_id < 0) {
        printf("Get BLE device failed.\n");
        return -1;
    }

    struct hci_dev_info bleDevInfo;
    int ret = hci_devinfo(g_dev_id, &bleDevInfo);
    if (ret < 0) {
        printf("BleDriverInit Failed to get HCI device info.\n");
        return -1;
    } else {
        printf("BleDriverInit hci_devinfo is name %s. \n", bleDevInfo.name);
    }

    return 0;
}

static int BleDriverDeinit(void* priv)
{
    return 0;
}

static int BleSetParam(SetParamsType paramType, void* priv)
{
    priv = priv;
    return 0;
}

static int BleGetParam(GetParamsType paramType, void* result)
{
    char** name = (char**)result;

    if (paramType == GET_DEV_NAME) {
        *name = "Send Test";
    }

    return 0;
}

static int BleStartAdv(void* priv)
{
    BleAdvParams* params = (BleAdvParams*)priv;
    // start advertise
    uint16_t min = htobs(params->minInterval);  // 20ms
    uint16_t max = htobs(params->maxInterval);  // 20ms
    uint8_t chan_map = params->advChannel;      // CHANNEL_37 | CHANNEL_38 | CHANNEL_39

    // open hci dev
    int device = hci_open_dev(g_dev_id);
    if (device < 0) {
        perror("Failed to open HCI device.");
        return -1;
    }

    int ret;
    // set advertise disable before set advertise parameters
    (void)hci_le_set_advertise_enable(device, 0, 1000);
    // set advertise parameters
    ret = hci_le_set_advertise_parameters(device, min, max, chan_map, 1000);
    if (ret != 0) {
        perror("Failed to set advertise parameters.");
        return -1;
    }
    // set advertise enable
    ret = hci_le_set_advertise_enable(device, 1, 1000);
    if (ret != 0) {
        perror("Failed to set advertise enable.");
        return -1;
    }
    hci_close_dev(device);
    return 0;
}

static int BleSetAdvData(void* priv)
{
    BleAdvData* params = (BleAdvData*)priv;
    int ret, device;
    device = hci_open_dev(g_dev_id);
    if (device < 0) {
        perror("Failed to open HCI device.");
        return -1;
    }
    // set advertise data
    ret = hci_le_set_advertise_data(device, params->advDataLen, params->advData, 1000);
    if (ret != 0) {
        perror("Failed to set advertise data.");
        return -1;
    }

    // set scan response data
    ret = hci_le_set_scan_response_data(device, params->rspDataLen, params->rspData, 1000);
    if (ret != 0) {
        perror("Failed to set scan response data.");
        return -1;
    }
    hci_close_dev(device);
    return 0;
}

static int BleStopAdv(void* priv)
{
    priv = priv;
    int device = hci_open_dev(g_dev_id);
    if (device < 0) {
        perror("Failed to open HCI device.");
        return -1;
    }
    int ret = stop_advertise(device);
    if (ret != 0) {
        perror("Failed to stop advertise.");
        hci_close_dev(device);
        return -1;
    }
    hci_close_dev(device);
    return 0;
}

static int BleStartScan(void* priv)
{
    priv = priv;
    uint8_t type = 1;                   // active scanning
    uint16_t interval = htobs(0x03C0);  // 600ms
    uint16_t window = htobs(0x0100);    // 160ms

    int device = hci_open_dev(g_dev_id);
    if (device < 0) {
        perror("Failed to open HCI device.");
        return -1;
    }

    int ret = start_scan(device, type, interval, window, HCI_FILTER_DUP_FLAG);
    if (ret != 0) {
        perror("Failed to start scan.");
        hci_close_dev(device);
        return -1;
    }
    hci_close_dev(device);

    pthread_create(&g_scan_pt, NULL, (void*)start_get_result, NULL);
    pthread_detach(g_scan_pt);
    return 0;
}

static int BleStopScan(void* priv)
{
    priv = priv;
    stop_get_result();
    int device = hci_open_dev(g_dev_id);
    if (device < 0) {
        perror("Failed to open HCI device.");
        return -1;
    }
    int ret = stop_scan(device, HCI_FILTER_DUP_FLAG);
    if (ret != 0) {
        perror("Failed to stop scan.");
        hci_close_dev(device);
        return -1;
    }
    hci_close_dev(device);
    return 0;
}

static int l2cap_le_att_connect(bdaddr_t* src, bdaddr_t* dst, uint8_t dst_type, int sec)
{
#define ATT_CID 4
    int sock;
    struct sockaddr_l2 srcaddr, dstaddr;
    struct bt_security btsec;

    sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock < 0) {
        perror("Failed to create L2CAP socket");
        return -1;
    }

    /* Set up source address */
    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.l2_family = AF_BLUETOOTH;
    srcaddr.l2_cid = htobs(ATT_CID);
    srcaddr.l2_bdaddr_type = 0;
    bacpy(&srcaddr.l2_bdaddr, src);

    if (bind(sock, (struct sockaddr*)&srcaddr, sizeof(srcaddr)) < 0) {
        perror("Failed to bind L2CAP socket");
        close(sock);
        return -1;
    }

    /* Set the security level */
    memset(&btsec, 0, sizeof(btsec));
    btsec.level = sec;
    if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &btsec, sizeof(btsec)) != 0) {
        fprintf(stderr, "Failed to set L2CAP security level\n");
        close(sock);
        return -1;
    }

    /* Set up destination address */
    memset(&dstaddr, 0, sizeof(dstaddr));
    dstaddr.l2_family = AF_BLUETOOTH;
    dstaddr.l2_cid = htobs(ATT_CID);
    dstaddr.l2_bdaddr_type = dst_type;
    bacpy(&dstaddr.l2_bdaddr, dst);

    printf("l2cap_le_att_connect connecting to device...");

    if (connect(sock, (struct sockaddr*)&dstaddr, sizeof(dstaddr)) < 0) {
        printf(" Failed \n");
        close(sock);
        return -1;
    }

    printf(" Done \n");
    return sock;
}

static void att_disconnect_cb(int err, void* user_data)
{
    printf("Device disconnected: %s\n", strerror(err));
    mainloop_quit();
}

static void notify_cb(uint16_t value_handle, const uint8_t* value, uint16_t length, void* user_data)
{
    printf("\n\tHandle Value Not/Ind: 0x%04x - ", value_handle);

    if (length == 0) {
        printf("(0 bytes)\n");
        return;
    }

    printf("(%u bytes): ", length);

    if (g_channel_cb && g_channel_cb->ReceiveData) {
        g_channel_cb->ReceiveData(CHANNEL_BLE, NULL, (unsigned char*)value, length);
    }
}

static void register_notify_cb(uint16_t att_ecode, void* user_data)
{
    if (att_ecode) {
        printf("Failed to register notify handler "
               "- error code: 0x%02x\n",
            att_ecode);
        if (g_channel_cb && g_channel_cb->ChannelStateChange) {
            g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT_FAIL, NULL, 0);
        }
        return;
    }
    struct client* cli = user_data;
    cli->g_desc_handle = cli->g_notify_handle + 1;
    printf("g_nearby_handle = %02x \n", cli->g_desc_handle);
    uint8_t data[2] = {0x01, 0x00};
    if (!bt_gatt_client_write_without_response(
            g_nearbyCli->gatt, g_nearbyCli->g_desc_handle, false, data, sizeof(data))) {
        printf("Failed to set_desc without response\n");
        if (g_channel_cb && g_channel_cb->ChannelStateChange) {
            g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT_FAIL, NULL, 0);
        }
        return;
    }

    printf("Registered notify handler!\n");
    if (g_channel_cb && g_channel_cb->ChannelStateChange) {
        g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT, NULL, 0);
    }
}

static void get_neaby_char(struct gatt_db_attribute* attr, void* user_data)
{
    uint16_t handle, value_handle;
    uint8_t properties;
    uint16_t ext_prop;
    bt_uuid_t uuid;

    if (!gatt_db_attribute_get_char_data(attr, &handle, &value_handle, &properties, &ext_prop, &uuid)) {
        if (g_channel_cb && g_channel_cb->ChannelStateChange) {
            g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT_FAIL, NULL, 0);
        }
        return;
    }

    struct client* cli = user_data;
    bt_uuid_t uuid128;
    char uuid_str[37];
    bt_uuid_to_uuid128(&uuid, &uuid128);
    bt_uuid_to_string(&uuid128, uuid_str, sizeof(uuid_str));
    if (strcasecmp(uuid_str, "00002a00-0000-1000-8000-00805f9b34fb") == 0) {
        cli->g_nearby_handle = value_handle;
        printf("g_nearby_handle = %02x \n", cli->g_nearby_handle);
    } else if (strcasecmp(uuid_str, "00002a01-0000-1000-8000-00805f9b34fb") == 0) {
        cli->g_notify_handle = value_handle;
        printf("g_notify_handle = %02x \n", cli->g_notify_handle);
        bt_gatt_client_register_notify(
            g_nearbyCli->gatt, cli->g_notify_handle, register_notify_cb, notify_cb, user_data, NULL);
    }
}

static void get_neaby_service(struct gatt_db_attribute* attr, void* user_data)
{
    gatt_db_service_foreach_char(attr, get_neaby_char, user_data);
}

static void ready_cb(bool success, uint8_t att_ecode, void* user_data)
{
    struct client* cli = user_data;

    if (!success) {
        printf("GATT discovery procedures failed - error code: 0x%02x\n", att_ecode);
        if (g_channel_cb && g_channel_cb->ChannelStateChange) {
            g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT_FAIL, NULL, 0);
        }
        return;
    }

    printf("GATT discovery procedures complete %p\n", cli->gatt);
    bt_uuid_t uuid;

    /* Handle the HID services */
    bt_uuid16_create(&uuid, 0xfe35);
    gatt_db_foreach_service(cli->db, &uuid, get_neaby_service, cli);
}

static void log_service_event(struct gatt_db_attribute* attr, const char* str)
{
    char uuid_str[MAX_LEN_UUID_STR];
    bt_uuid_t uuid;
    uint16_t start, end;

    gatt_db_attribute_get_service_uuid(attr, &uuid);
    bt_uuid_to_string(&uuid, uuid_str, sizeof(uuid_str));

    gatt_db_attribute_get_service_handles(attr, &start, &end);

    printf("%s - UUID: %s start: 0x%04x end: 0x%04x\n", str, uuid_str, start, end);
}

static void service_added_cb(struct gatt_db_attribute* attr, void* user_data)
{
    log_service_event(attr, "Service Added");
}

static void service_removed_cb(struct gatt_db_attribute* attr, void* user_data)
{
    log_service_event(attr, "Service Removed");
}

static void service_changed_cb(uint16_t start_handle, uint16_t end_handle, void* user_data)
{
    printf("\nService Changed handled - start: 0x%04x end: 0x%04x\n", start_handle, end_handle);
}
static struct client* client_create(int fd, uint16_t mtu)
{
    struct client* cli;

    cli = new0(struct client, 1);
    if (!cli) {
        printf("Failed to allocate memory for client\n");
        return NULL;
    }

    cli->att = bt_att_new(fd, false);
    if (!cli->att) {
        printf("Failed to initialze ATT transport layer\n");
        bt_att_unref(cli->att);
        free(cli);
        return NULL;
    }

    if (!bt_att_set_close_on_unref(cli->att, true)) {
        printf("Failed to set up ATT transport layer\n");
        bt_att_unref(cli->att);
        free(cli);
        return NULL;
    }

    if (!bt_att_register_disconnect(cli->att, att_disconnect_cb, NULL, NULL)) {
        printf("Failed to set ATT disconnect handler\n");
        bt_att_unref(cli->att);
        free(cli);
        return NULL;
    }

    cli->fd = fd;
    cli->db = gatt_db_new();
    if (!cli->db) {
        printf("Failed to create GATT database\n");
        bt_att_unref(cli->att);
        free(cli);
        return NULL;
    }

    cli->gatt = bt_gatt_client_new(cli->db, cli->att, mtu);
    if (!cli->gatt) {
        printf("Failed to create GATT client\n");
        gatt_db_unref(cli->db);
        bt_att_unref(cli->att);
        free(cli);
        return NULL;
    }

    gatt_db_register(cli->db, service_added_cb, service_removed_cb, NULL, NULL);

    bt_gatt_client_ready_register(cli->gatt, ready_cb, cli, NULL);
    bt_gatt_client_set_service_changed(cli->gatt, service_changed_cb, cli, NULL);
    /* bt_gatt_client already holds a reference */
    gatt_db_unref(cli->db);
    return cli;
}

static void client_destroy(struct client* cli)
{
    bt_gatt_client_unref(cli->gatt);
    bt_att_unref(cli->att);
    free(cli);
}

int GattcConnectByAddress(void* priv)
{
    struct connect* con = (struct connect*)priv;
    int sec = BT_SECURITY_LOW;
    uint16_t mtu = 503;
    int fd;

    char srcAddr[18];
    char dstAddr[18];
    // need to change here.
    ba2str(&con->src, srcAddr);
    ba2str(&con->dst, dstAddr);
    printf("gattc connect from %s to %s dstType %d\n", srcAddr, dstAddr, con->dstType);
    mainloop_init();

    fd = l2cap_le_att_connect(&con->src, &con->dst, con->dstType, sec);
    if (fd < 0) {
        if (g_channel_cb && g_channel_cb->ChannelStateChange) {
            g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT_FAIL, NULL, 0);
        }
    }

    g_nearbyCli = client_create(fd, mtu);
    if (!g_nearbyCli) {
        close(fd);
        if (g_channel_cb && g_channel_cb->ChannelStateChange) {
            g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT_FAIL, NULL, 0);
        }
        return -1;
    }

    printf("BT_INFO Gattc connect\n");
    mainloop_run();
    client_destroy(g_nearbyCli);
    printf("BT_INFO Gattc exit\n");
    if (g_channel_cb && g_channel_cb->ChannelStateChange) {
        g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_DISCONNECT, NULL, 0);
    }
    return 0;
}

static int BleConnect(void* priv)
{
    static struct connect con;
    if (priv == NULL) {
        return -1;
    }
    BleConnectParam* parm = (BleConnectParam*)priv;

    char address[18];
    // need to change here.
    sprintf(address,
        "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
        parm->addr[5],
        parm->addr[4],
        parm->addr[3],
        parm->addr[2],
        parm->addr[1],
        parm->addr[0]);
    str2ba(address, &con.dst);

    if (parm->addrType == 0) {
        con.dstType = BDADDR_LE_PUBLIC;
    } else {
        // random connect
        con.dstType = BDADDR_LE_RANDOM;
    }

    struct hci_dev_info bleDevInfo;
    int ret = hci_devinfo(g_dev_id, &bleDevInfo);
    if (ret < 0) {
        printf("Failed to get HCI device info.\n");
        return -1;
    } else {
        printf("hci_devinfo is name %s. \n", bleDevInfo.name);
    }
    bacpy(&con.src, &bleDevInfo.bdaddr);

    pthread_t pt;
    pthread_create(&pt, NULL, (void*)GattcConnectByAddress, &con);

    return ret;
}

static int BleDisconnect(void* priv)
{
    printf("bt debug info BleDisconnect.\n");
    mainloop_quit();
    return 0;
}

int BleRegisterChannelCallback(ChannelCallback* channelCallback)
{
    if (channelCallback) {
        g_channel_cb = channelCallback;
        return 0;
    }
    return -1;
}

static int hci_le_set_advertise_parameters(
    int dd, uint16_t min_interval, uint16_t max_interval, uint8_t chan_map, int to)
{
    struct hci_request rq;
    le_set_advertising_parameters_cp param_cp;
    uint8_t status;

    memset(&param_cp, 0, sizeof(param_cp));
    param_cp.min_interval = min_interval;
    param_cp.max_interval = max_interval;
    param_cp.advtype = 0x00;  // ADV_IND
    param_cp.own_bdaddr_type = HCI_DEV_ADDRTYPE;
    param_cp.chan_map = chan_map;
    param_cp.filter = 0x00;  // Process scan and connection requests from all devices

    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    rq.cparam = &param_cp;
    rq.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    if (hci_send_req(dd, &rq, to) < 0)
        return -1;

    if (status) {
        errno = EIO;
        return -1;
    }

    return 0;
}

static int hci_le_set_advertise_data(int dd, uint8_t length, uint8_t* data, int to)
{
    struct hci_request rq;
    le_set_advertising_data_cp param_cp;
    uint8_t status;

    memset(&param_cp, 0, sizeof(param_cp));
    param_cp.length = length;
    if (data)
        memcpy(param_cp.data, data, length);

    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISING_DATA;
    rq.cparam = &param_cp;
    rq.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    if (hci_send_req(dd, &rq, to) < 0)
        return -1;

    if (status) {
        errno = EIO;
        return -1;
    }

    return 0;
}

static int hci_le_set_scan_response_data(int dd, uint8_t length, uint8_t* data, int to)
{
    struct hci_request rq;
    le_set_advertising_data_cp param_cp;
    uint8_t status;

    memset(&param_cp, 0, sizeof(param_cp));
    param_cp.length = length;
    if (data)
        memcpy(param_cp.data, data, length);

    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_SCAN_RESPONSE_DATA;
    rq.cparam = &param_cp;
    rq.clen = LE_SET_SCAN_RESPONSE_DATA_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    if (hci_send_req(dd, &rq, to) < 0)
        return -1;

    if (status) {
        errno = EIO;
        return -1;
    }

    return 0;
}

static int stop_advertise(int dd)
{
    // set advertise disable
    int ret = hci_le_set_advertise_enable(dd, 0, 1000);
    if (ret != 0) {
        perror("Failed to set advertise disable.");
        return -1;
    }

    return 0;
}

static int start_scan(int dd, uint8_t type, uint16_t interval, uint16_t window, uint8_t filter_dup)
{
    // set scan disable before set scan parameters
    (void)hci_le_set_scan_enable(dd, 0, filter_dup, 1000);

    // set scan parameters
    int ret = hci_le_set_scan_parameters(dd, type, interval, window, HCI_DEV_ADDRTYPE, filter_dup, 1000);
    if (ret != 0) {
        perror("Failed to set scan parameters.");
        return -1;
    }

    // set scan enable
    ret = hci_le_set_scan_enable(dd, 1, filter_dup, 1000);
    if (ret != 0) {
        perror("Failed to set scan enable.");
        return -1;
    }

    return 0;
}

static int stop_scan(int dd, uint8_t filter_dup)
{
    // set scan disable
    int ret = hci_le_set_scan_enable(dd, 0, filter_dup, 1000);
    if (ret != 0) {
        perror("Failed to set advertise disable.");
        return -1;
    }
    return 0;
}

static int scan_callback(BleScanResult* scan_result)
{
    if (scan_result && g_channel_cb && g_channel_cb->ScanCallback) {
#if 0
        int i;
        printf("scan result");
        printf("rssi %d adv type %d mac type %d mac %02X:%02X:%02X:%02X:%02X:%02X",
            scan_result->rssi,
            scan_result->evtType,
            scan_result->addrType,
            scan_result->addr[5],
            scan_result->addr[4],
            scan_result->addr[3],
            scan_result->addr[2],
            scan_result->addr[1],
            scan_result->addr[0]);
        printf(" data len %d::\n", scan_result->len);
        for (i = 0; i < scan_result->len; i++) {
            printf("0x%02X ", scan_result->data[i]);
        }
        printf("\n");
#endif
        g_channel_cb->ScanCallback(CHANNEL_BLE, scan_result);
    }
    return 0;
}

static int hci_le_get_advertising_reports(int dd)
{
    struct hci_filter old_opt;
    socklen_t old_opt_len = sizeof(old_opt);

    // get old socket options
    if (getsockopt(dd, SOL_HCI, HCI_FILTER, &old_opt, &old_opt_len) < 0) {
        printf("Could not get socket options\n");
        return -1;
    }

    // set new socket options
    struct hci_filter new_opt;
    hci_filter_clear(&new_opt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_opt);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_opt);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &new_opt, sizeof(new_opt)) < 0) {
        printf("Could not set socket options\n");
        return -1;
    }

    unsigned char buffer[HCI_MAX_EVENT_SIZE];
    int len = 0;
    evt_le_meta_event* meta_event = (evt_le_meta_event*)(buffer + HCI_EVENT_HDR_SIZE + 1);
    le_advertising_info* info;
    BleScanResult scan_result;
    uint8_t reports_count;
    uint8_t* offset;
    // start scan
    printf("LE get scan report...\n");
    while (g_scan_running) {
        if ((len = read(dd, buffer, sizeof(buffer))) < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            goto done;
        } else {
            len -= (1 + HCI_EVENT_HDR_SIZE);
            if (meta_event->subevent != EVT_LE_ADVERTISING_REPORT)
                goto done;

            reports_count = meta_event->data[0];
            offset = meta_event->data + 1;
            for (uint8_t i = 0; i < reports_count; i++) {
                info = (le_advertising_info*)(offset);
                memset(&scan_result, 0, sizeof(scan_result));
                scan_result.evtType = info->evt_type;
                scan_result.addrType = info->bdaddr_type;
                memcpy(&scan_result.addr, &info->bdaddr, sizeof(info->bdaddr));
                scan_result.len = info->length;
                memcpy(&scan_result.data[0], &info->data[0], info->length);
                scan_result.rssi = info->data[info->length];
                scan_callback(&scan_result);
                offset = info->data + info->length + 2;
            }
        }
    }
done:
    printf("LE get scan stop. \n");
    setsockopt(dd, SOL_HCI, HCI_FILTER, &old_opt, sizeof(old_opt));

    if (len < 0)
        return -1;
    return 0;
}

static void start_get_result(void)
{
    if (g_scan_running == 1) {
        return;
    }
    g_scan_running = 1;
    int device = hci_open_dev(g_dev_id);
    if (device < 0) {
        perror("Failed to open HCI device.");
        return;
    }
    int ret = hci_le_get_advertising_reports(device);
    if (ret < 0) {
        perror("Could not get scan reports");
    }
    hci_close_dev(device);
    return;
}

static void stop_get_result(void)
{
    g_scan_running = 0;
    sleep(1);
}

static int BleSendData(unsigned char* data, int dataLen, void* priv)
{
    if (!g_nearbyCli || !g_nearbyCli->g_nearby_handle || !data || !dataLen) {
        printf("BleSendData fail.\n");
        return -1;
    }
    printf("BLE SendData %d.\n", dataLen);

    if (!bt_gatt_client_is_ready(g_nearbyCli->gatt)) {
        printf("GATT client not initialized\n");
        return -1;
    }

    if (!bt_gatt_client_write_without_response(g_nearbyCli->gatt, g_nearbyCli->g_nearby_handle, false, data, dataLen)) {
        printf("Failed to initiate write without response "
               "procedure\n");
        return -1;
    }
    return 0;
}

const struct FcDriverAdaptOps g_fcAdaptBleOps = {
    .name = "ble",
    .desc = "ble driver",
    .Init = BleDriverInit,
    .Deinit = BleDriverDeinit,
    .SetParam = BleSetParam,
    .GetParam = BleGetParam,
    .StartAdv = BleStartAdv,
    .StopAdv = BleStopAdv,
    .SetAdvData = BleSetAdvData,
    .StartScan = BleStartScan,
    .StopScan = BleStopScan,
    .Connect = BleConnect,
    .Disconnect = BleDisconnect,
    .RegisterChannelCallback = BleRegisterChannelCallback,
    .SendData = BleSendData,
};
