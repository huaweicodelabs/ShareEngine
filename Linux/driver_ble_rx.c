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

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/l2cap.h"
#include "lib/uuid.h"

#include "src/shared/mainloop.h"
#include "src/shared/util.h"
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/timeout.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-server.h"

#define HCI_DEV_ADDRTYPE LE_PUBLIC_ADDRESS
#define HCI_FILTER_DUP_FLAG 0x00
#define UUID_NEARBY_SERVICE 0xfe35
#define UUID_NEARBY_CHARACTERISTIC_EXCHANGE 0x2a00
#define UUID_NEARBY_CHARACTERISTIC_NOTIFICATION 0x2a01

typedef struct {
    uint8_t evt_type;
    uint8_t bdaddr_type;
    bdaddr_t bdaddr;
    uint8_t length;
    uint8_t data[31];
    int8_t rssi;
} le_advertising_report;

struct server {
    int fd;
    struct bt_att* att;
    struct gatt_db* db;
    struct bt_gatt_server* gatt;

    uint16_t nb_exchange_handle;
    uint16_t nb_notify_handle;
};

int g_dev_id;         // hciX dev id
pthread_t g_scan_pt;  // scan thread id
int g_scan_running;   // scan thread runing status
ChannelCallback* g_channel_cb;
struct server* g_server = NULL;

static int stop_advertise(int dd);
static int start_scan(int dd, uint8_t type, uint16_t interval, uint16_t window, uint8_t filter_dup);
static int stop_scan(int dd, uint8_t filter_dup);
static void start_get_result(void);
static void stop_get_result(void);
static int hci_le_set_advertise_parameters(
    int dd, uint16_t min_interval, uint16_t max_interval, uint8_t chan_map, int to);
static int hci_le_set_advertise_data(int dd, uint8_t length, uint8_t* data, int to);
static int hci_le_set_scan_response_data(int dd, uint8_t length, uint8_t* data, int to);

int BleSendResponse(unsigned char* value, int valLen)
{
    bt_gatt_server_send_notification(g_server->gatt, g_server->nb_notify_handle, value, valLen);
    return 0;
}

static void att_disconnect_cb(int err, void* user_data)
{
    printf("Device disconnected: %s \n", strerror(err));
    mainloop_quit();
}

static void gap_nearby_read_cb(struct gatt_db_attribute* attrib, unsigned int id, uint16_t offset, uint8_t opcode,
    struct bt_att* att, void* user_data)
{
    size_t len = 0;
    uint8_t error = 0;
    const uint8_t* value = NULL;

    printf("gap_nearby_read_cb Read called\n");
    gatt_db_attribute_read_result(attrib, id, error, value, len);
}

static void gap_nearby_write_cb(struct gatt_db_attribute* attrib, unsigned int id, uint16_t offset,
    const uint8_t* value, size_t len, uint8_t opcode, struct bt_att* att, void* user_data)
{
    uint8_t error = 0;

    if (value == NULL || len == 0) {
        // error = BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN;
        goto done;
    }
    printf("gap_nearby_write_cb Write called data len is %d\n", (int)len);

    if (g_channel_cb && g_channel_cb->ReceiveData) {
        g_channel_cb->ReceiveData(CHANNEL_BLE, NULL, (unsigned char *)value, len);
    }

done:
    gatt_db_attribute_write_result(attrib, id, error);
}

static void nearby_service(struct server* server)
{
    bt_uuid_t uuid;
    struct gatt_db_attribute *service, *exchange_handle, *nofity_handle;

    /* Add Heart Rate Service */
    bt_uuid16_create(&uuid, UUID_NEARBY_SERVICE);
    service = gatt_db_add_service(server->db, &uuid, true, 8);

    /* HR Measurement Characteristic */
    bt_uuid16_create(&uuid, UUID_NEARBY_CHARACTERISTIC_EXCHANGE);
    exchange_handle = gatt_db_service_add_characteristic(service,
        &uuid,
        BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
        BT_GATT_CHRC_PROP_READ | BT_GATT_CHRC_PROP_WRITE,
        gap_nearby_read_cb,
        gap_nearby_write_cb,
        NULL);
    server->nb_exchange_handle = gatt_db_attribute_get_handle(exchange_handle);

    /* HR Measurement Characteristic */
    bt_uuid16_create(&uuid, UUID_NEARBY_CHARACTERISTIC_NOTIFICATION);
    nofity_handle = gatt_db_service_add_characteristic(
        service, &uuid, BT_ATT_PERM_NONE, BT_GATT_CHRC_PROP_NOTIFY, NULL, NULL, NULL);
    server->nb_notify_handle = gatt_db_attribute_get_handle(nofity_handle);

    gatt_db_service_set_active(service, true);
}

static struct server* server_create(int fd, uint16_t mtu)
{
    struct server* server;

    server = new0(struct server, 1);
    if (!server) {
        fprintf(stderr, "Failed to allocate memory for server\n");
        return NULL;
    }

    server->att = bt_att_new(fd, false);
    if (!server->att) {
        fprintf(stderr, "Failed to initialze ATT transport layer\n");
        goto fail;
    }

    if (!bt_att_set_close_on_unref(server->att, true)) {
        fprintf(stderr, "Failed to set up ATT transport layer\n");
        goto fail;
    }

    if (!bt_att_register_disconnect(server->att, att_disconnect_cb, NULL, NULL)) {
        fprintf(stderr, "Failed to set ATT disconnect handler\n");
        goto fail;
    }

    server->fd = fd;
    server->db = gatt_db_new();
    if (!server->db) {
        fprintf(stderr, "Failed to create GATT database\n");
        goto fail;
    }

    server->gatt = bt_gatt_server_new(server->db, server->att, mtu, 0);
    if (!server->gatt) {
        fprintf(stderr, "Failed to create GATT server\n");
        goto fail;
    }

    /* bt_gatt_server already holds a reference */
    nearby_service(server);

    return server;

fail:
    gatt_db_unref(server->db);
    bt_att_unref(server->att);
    free(server);

    return NULL;
}

static int l2cap_le_att_listen_and_accept(bdaddr_t* src, int sec, uint8_t src_type)
{
#define ATT_CID 4
    int sk, nsk;
    struct sockaddr_l2 srcaddr, addr;
    socklen_t optlen;
    struct bt_security btsec;
    char ba[18];

    sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sk < 0) {
        perror("Failed to create L2CAP socket");
        return -1;
    }

    /* Set up source address */
    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.l2_family = AF_BLUETOOTH;
    srcaddr.l2_cid = htobs(ATT_CID);
    srcaddr.l2_bdaddr_type = src_type;
    bacpy(&srcaddr.l2_bdaddr, src);

    if (bind(sk, (struct sockaddr*)&srcaddr, sizeof(srcaddr)) < 0) {
        perror("Failed to bind L2CAP socket");
        goto fail;
    }

    /* Set the security level */
    memset(&btsec, 0, sizeof(btsec));
    btsec.level = sec;
    if (setsockopt(sk, SOL_BLUETOOTH, BT_SECURITY, &btsec, sizeof(btsec)) != 0) {
        fprintf(stderr, "Failed to set L2CAP security level\n");
        goto fail;
    }

    if (listen(sk, 10) < 0) {
        perror("Listening on socket failed");
        goto fail;
    }

    printf("BT log Started listening on ATT channel. Waiting for connections\n");
    memset(&addr, 0, sizeof(addr));
    optlen = sizeof(addr);
    nsk = accept(sk, (struct sockaddr*)&addr, &optlen);
    if (nsk < 0) {
        perror("Accept failed");
        goto fail;
    }

    ba2str(&addr.l2_bdaddr, ba);
    printf("Connect from %s\n", ba);
    close(sk);
    return nsk;
fail:
    close(sk);
    return -1;
}

void StopGattServer(void)
{
    printf("Debug StopGattServer\n");
    mainloop_quit();
}

static void server_destroy(struct server* server)
{
    bt_gatt_server_unref(server->gatt);
    gatt_db_unref(server->db);
}

int StartGattServer(void)
{
    printf("Debug StartGattServer\n");
    bdaddr_t src_addr;
    int fd;
    int sec = BT_SECURITY_LOW;
    uint8_t src_type = BDADDR_LE_PUBLIC;
    uint16_t mtu = 503;
    printf("BT log Start gatt server\n");

    mainloop_init();
    bacpy(&src_addr, BDADDR_ANY);
    fd = l2cap_le_att_listen_and_accept(&src_addr, sec, src_type);
    if (fd < 0) {
        printf("Failed to accept L2CAP ATT connection\n");
        return -1;
    }

    g_server = server_create(fd, mtu);
    if (!g_server) {
        close(fd);
        return -1;
    }
    printf("Running GATT server\n");
    if (g_channel_cb && g_channel_cb->ChannelStateChange) {
        g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_CONNECT, NULL, 0);
    }
    mainloop_run();
    printf("GATT server down...\n");
    server_destroy(g_server);
    if (g_channel_cb && g_channel_cb->ChannelStateChange) {
        g_channel_cb->ChannelStateChange(CHANNEL_BLE, EVENT_DISCONNECT, NULL, 0);
    }
    return 0;
}

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

    // register gattservice here
    pthread_t pt;
    pthread_create(&pt, NULL, (void*)StartGattServer, NULL);
    return 0;
}

static int BleDriverDeinit(void* priv)
{
    StopGattServer();
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
        *name = "Receive Test";
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

static int BleNotifyData(unsigned char* data, int dataLen, void* priv)
{
    printf("BleNotifyData come in\n");
    bt_gatt_server_send_notification(g_server->gatt, g_server->nb_notify_handle, data, dataLen);
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
    .RegisterChannelCallback = BleRegisterChannelCallback,
    .NotifyData = BleNotifyData,
};