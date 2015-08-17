#ifndef __BLE_EVT_PRINTOUT_H__
#define __BLE_EVT_PRINTOUT_H__

#ifdef DEBUG

#include <stdio.h>

#include "ble.h"

#define print_ble_evt(p_ble_evt) \
do                                                                                      \
    {                                                                                   \
    static const char * s_common_evts[] =                                               \
    {                                                                                   \
        "BLE_EVT_TX_COMPLETE",                                                          \
        "BLE_EVT_USER_MEM_REQUEST",                                                     \
        "BLE_EVT_USER_MEM_RELEASE"                                                      \
    };                                                                                  \
                                                                                        \
    static const char * s_gap_evts[] =                                                  \
    {                                                                                   \
      "BLE_GAP_EVT_CONNECTED",                                                          \
      "BLE_GAP_EVT_DISCONNECTED",                                                       \
      "BLE_GAP_EVT_CONN_PARAM_UPDATE",                                                  \
      "BLE_GAP_EVT_SEC_PARAMS_REQUEST",                                                 \
      "BLE_GAP_EVT_SEC_INFO_REQUEST",                                                   \
      "BLE_GAP_EVT_PASSKEY_DISPLAY",                                                    \
      "BLE_GAP_EVT_AUTH_KEY_REQUEST",                                                   \
      "BLE_GAP_EVT_AUTH_STATUS",                                                        \
      "BLE_GAP_EVT_CONN_SEC_UPDATE",                                                    \
      "BLE_GAP_EVT_TIMEOUT",                                                            \
      "BLE_GAP_EVT_RSSI_CHANGED",                                                       \
      "BLE_GAP_EVT_ADV_REPORT",                                                         \
      "BLE_GAP_EVT_SEC_REQUEST",                                                        \
      "BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST",                                          \
      "BLE_GAP_EVT_SCAN_REQ_REPORT"                                                     \
    };                                                                                  \
                                                                                        \
    static const char * s_l2cap_evts[] =                                                \
    {                                                                                   \
        "BLE_L2CAP_EVT_RX"                                                              \
    };                                                                                  \
                                                                                        \
    static const char * s_gatts_evts[] =                                                \
    {                                                                                   \
        "BLE_GATTS_EVT_WRITE",                                                          \
        "BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST",                                           \
        "BLE_GATTS_EVT_SYS_ATTR_MISSING",                                               \
        "BLE_GATTS_EVT_HVC",                                                            \
        "BLE_GATTS_EVT_SC_CONFIRM",                                                     \
        "BLE_GATTS_EVT_TIMEOUT"                                                         \
    };                                                                                  \
                                                                                        \
    static const char * s_gattc_evts[] =                                                \
    {                                                                                   \
        "BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP",                                             \
        "BLE_GATTC_EVT_REL_DISC_RSP",                                                   \
        "BLE_GATTC_EVT_CHAR_DISC_RSP",                                                  \
        "BLE_GATTC_EVT_DESC_DISC_RSP",                                                  \
        "BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP",                                      \
        "BLE_GATTC_EVT_READ_RSP",                                                       \
        "BLE_GATTC_EVT_CHAR_VALS_READ_RSP",                                             \
        "BLE_GATTC_EVT_WRITE_RSP",                                                      \
        "BLE_GATTC_EVT_HVX",                                                            \
        "BLE_GATTC_EVT_TIMEOUT"                                                         \
    };                                                                                  \
                                                                                        \
    char * evt_str = 0;                                                                 \
                                                                                        \
    if (p_ble_evt->header.evt_id >= BLE_L2CAP_EVT_BASE)                                 \
    {                                                                                   \
        /* L2CAP event */                                                               \
        evt_str = (char *) s_l2cap_evts[p_ble_evt->header.evt_id - BLE_L2CAP_EVT_BASE]; \
    }                                                                                   \
    else if (p_ble_evt->header.evt_id >= BLE_GATTS_EVT_BASE)                            \
    {                                                                                   \
        /* GATT Server event */                                                         \
        evt_str = (char *) s_gatts_evts[p_ble_evt->header.evt_id - BLE_GATTS_EVT_BASE]; \
    }                                                                                   \
    else if (p_ble_evt->header.evt_id >= BLE_GATTC_EVT_BASE)                            \
    {                                                                                   \
        /* GATT Client event */                                                         \
        evt_str = (char *) s_gattc_evts[p_ble_evt->header.evt_id - BLE_GATTC_EVT_BASE]; \
    }                                                                                   \
    else if (p_ble_evt->header.evt_id >= BLE_GAP_EVT_BASE)                              \
    {                                                                                   \
        /* GAP event */                                                                 \
        evt_str = (char *) s_gap_evts[p_ble_evt->header.evt_id - BLE_GAP_EVT_BASE];     \
    }                                                                                   \
    else if (p_ble_evt->header.evt_id >= BLE_EVT_BASE)                                  \
    {                                                                                   \
        /* Common event */                                                              \
        evt_str = (char *) s_common_evts[p_ble_evt->header.evt_id - BLE_EVT_BASE];      \
    }                                                                                   \
                                                                                        \
    if (evt_str != 0)                                                                   \
    {                                                                                   \
        printf("%s\r\n", evt_str);                                                      \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        printf("INVALID EVENT\r\n");                                                    \
    }                                                                                   \
} while(0);
#else
#define print_ble_evt(...)
#endif /* DEBUG */

#endif /* __BLE_EVT_PRINTOUT_H__ */
