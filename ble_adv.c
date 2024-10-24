/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * @ingroup     ble_adv
 *
 * @{
 * @brief   Implementation of the ble_adv library
 * @file
 */
#include "ble_adv.h"

#include <endian.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/**
 * @name    EIR entry types
 * @{
 */
#define EIR_FLAGS                                   0x01  /**< flags */
#define EIR_UUID16_SOME                             0x02  /**< 16-bit UUID, more available */
#define EIR_UUID16_ALL                              0x03  /**< 16-bit UUID, all listed */
#define EIR_UUID32_SOME                             0x04  /**< 32-bit UUID, more available */
#define EIR_UUID32_ALL                              0x05  /**< 32-bit UUID, all listed */
#define EIR_UUID128_SOME                            0x06  /**< 128-bit UUID, more available */
#define EIR_UUID128_ALL                             0x07  /**< 128-bit UUID, all listed */
#define EIR_NAME_SHORT                              0x08  /**< shortened local name */
#define EIR_NAME_COMPLETE                           0x09  /**< complete local name */
#define EIR_TX_POWER                                0x0A  /**< transmit power level */
#define EIR_DEVICE_ID                               0x10  /**< device ID */
#define EIR_SERVICE_DATA                            0x16  /**< service data*/
#define EIR_URI                                     0x24  /**< URI */
#define EIR_MANUFACTURER_SPECIFIC_DATA              0xFF  /**< manufacturer specific data */
/** @} */

/**
 * @name    BLE event types
 * @{
 */
#define EVT_LE_CONN_COMPLETE                        0x01
#define EVT_LE_ADVERTISING_REPORT                   0x02
#define EVT_LE_CONN_UPDATE_COMPLETE                 0x03
#define EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE   0x04
/** @} */

/**
 * @brief   Parse the EIR data of the BLE advertisements
 * @param[out]      dest        Write decoded fields here
 * @param[in]       eir         Data to decode
 * @param[in]       eir_len     Length of @p eir
 * @retval  0                   Success
 * @retval -EPROTO              Invalid encoding detected
 * @retval -EOVERFLOW           EIR field larger than space in @p dest
 */
static int parse_eir(struct ble_adv *dest, const uint8_t *eir, size_t eir_len)
{
    dest->name_len = 0;
    dest->uri_len = 0;
    dest->tx_power = INT8_MAX;
    dest->has = 0;
    while (eir_len) {
        uint8_t field_len = eir[0];
        if (field_len == 0) {
            /* reached end of EIR */
            return 0;
        }

        /* consume length byte */
        eir++;
        eir_len--;

        if (field_len > eir_len) {
            /* format error */
            return -EPROTO;
        }

        /* fetch header, step data pointer */
        uint8_t header = eir[0];
        eir++;
        field_len--;
        eir_len--;
        switch (header) {
        default:
            break;
        case EIR_FLAGS:
            if (field_len >= 1) {
                dest->flags = *eir;
                dest->has |= BLE_ADV_HAS_FLAGS;
            }
            break;
        case EIR_NAME_SHORT:
        case EIR_NAME_COMPLETE:
            if (field_len) {
                if (sizeof(dest->name) < field_len + 1U) {
                    return -EOVERFLOW;
                }
                memcpy(dest->name, eir, field_len);
                dest->name[field_len] = '\0';
                dest->name_len = field_len;
            }
            break;
        case EIR_TX_POWER:
            if (field_len == 1) {
                dest->tx_power = (int8_t)eir[0];
            }
            break;
        case EIR_SERVICE_DATA:
            if (field_len >= 2) {
                if (field_len - 2U > sizeof(dest->service_data)) {
                    return -EOVERFLOW;
                }
                /* UUID16 may be unaligned */
                memcpy(&dest->service_uuid16, eir, sizeof(uint16_t));
                dest->service_uuid16 = le16toh(dest->service_uuid16);
                memcpy(dest->service_data, eir + 2, field_len - 2U);
                dest->service_data_len = field_len - 2U;
                dest->has |= BLE_ADV_HAS_SERVICE_DATA;
            }
            break;
        case EIR_MANUFACTURER_SPECIFIC_DATA:
            if (field_len >= 2) {
                if (field_len - 2U > sizeof(dest->ms_data)) {
                    return -EOVERFLOW;
                }
                /* UUID16 may be unaligned */
                memcpy(&dest->ms_uuid16, eir, sizeof(uint16_t));
                dest->ms_uuid16 = le16toh(dest->ms_uuid16);
                memcpy(dest->ms_data, eir + 2, field_len - 2U);
                dest->ms_data_len = field_len - 2U;
                dest->has |= BLE_ADV_HAS_MS_DATA;
            }
            break;
        case EIR_URI:
            if (field_len) {
                if (field_len + 1U > sizeof(dest->uri)) {
                    return -EOVERFLOW;
                }
                memcpy(dest->uri, eir, field_len);
                dest->uri_len = field_len;
                dest->uri[field_len] = '\0';
            }
            break;
        case EIR_UUID16_SOME:
        case EIR_UUID16_ALL:
            if (field_len >= sizeof(dest->uuid16)) {
                /* data may be unaligned */
                memcpy(&dest->uuid16, eir, sizeof(dest->uuid16));
                dest->uuid16 = le16toh(dest->uuid16);
                dest->has |= BLE_ADV_HAS_UUID16;
            }
            break;
        case EIR_UUID32_SOME:
        case EIR_UUID32_ALL:
            if (field_len >= sizeof(dest->uuid32)) {
                /* data may be unaligned */
                memcpy(&dest->uuid32, eir, sizeof(dest->uuid32));
                dest->uuid32 = le32toh(dest->uuid32);
                dest->has |= BLE_ADV_HAS_UUID32;
            }
            break;
        case EIR_UUID128_SOME:
        case EIR_UUID128_ALL:
            if (field_len >= sizeof(dest->uuid128)) {
                memcpy(&dest->uuid128, eir, sizeof(dest->uuid128));
                dest->has |= BLE_ADV_HAS_UUID128;
            }
            break;
        }
        eir += field_len;
        eir_len -= field_len;
    }

    return 0;
}

int ble_adv_read(int dev, struct ble_adv *dest)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    ssize_t len;

    if ((dev == -1) || !dest) {
        errno = EINVAL;
        return -1;
    }

    while (0 > (len = read(dev, buf, sizeof(buf)))) {
        if (errno == EINTR) {
            continue;
        }
        return -1;
    }

    evt_le_meta_event *meta;
    if ((size_t)len < sizeof(*meta) + 1 + HCI_EVENT_HDR_SIZE) {
        errno = EPROTO;
        return -1;
    }
    meta = (evt_le_meta_event *)(buf + 1 + HCI_EVENT_HDR_SIZE);

    if (meta->subevent != EVT_LE_ADVERTISING_REPORT) {
        errno = ENOENT;
        return -1;
    }

    le_advertising_info *info;
    info = (le_advertising_info *)(meta->data + 1);

    dest->addr[0] = info->bdaddr.b[5];
    dest->addr[1] = info->bdaddr.b[4];
    dest->addr[2] = info->bdaddr.b[3];
    dest->addr[3] = info->bdaddr.b[2];
    dest->addr[4] = info->bdaddr.b[1];
    dest->addr[5] = info->bdaddr.b[0];
    int err = parse_eir(dest, info->data, info->length);
    if (err) {
        errno = -err;
        return -1;
    }

    if (dest->name_len == 0) {
        const char fallback[] = "<unknown>";
        memcpy(dest->name, fallback, sizeof(fallback));
    }

    if (dest->uri_len == 0) {
        const char fallback[] = "";
        memcpy(dest->uri, fallback, sizeof(fallback));
    }

    dest->rssi = buf[len - 1];

    return 0;
}

static inline int use_public_address(unsigned flags)
{
    /* Using the public address if either no privacy is explicitly requested, or when scanning
     * passive only */
    return (flags & (BLE_ADV_SCAN_FLAG_PASSIVE | BLE_ADV_SCAN_FLAG_PUBLIC_ADDR));
}

int ble_adv_scan(int dev, unsigned flags)
{
    const uint8_t filter_policy = 0; /* don't use whitelist for filtering */
    const uint16_t interval = htobs(0x0010);
    const uint16_t window = htobs(0x0010);
    uint8_t scan_type = (flags & BLE_ADV_SCAN_FLAG_PASSIVE) ? 0 : 1;
    uint8_t own_type = use_public_address(flags) ? LE_PUBLIC_ADDRESS : LE_RANDOM_ADDRESS;

    if (dev == -1) {
        errno = EINVAL;
        return -1;
    }

    if (!(flags & BLE_ADV_SCAN_FLAG_ENABLED)) {
        if (hci_le_set_scan_enable(dev, 0, 0, 10000)) {
            return -1;
        }
        return 0;
    }

    if (hci_le_set_scan_parameters(dev, scan_type, interval, window,
                                   own_type, filter_policy, 10000))
    {
        if (errno == EIO) {
            /* maybe already scanning, trying to disable scanning first to be able to
             * apply the config */
            if (hci_le_set_scan_enable(dev, 0, 0, 10000)) {
                return -1;
            }
            if (hci_le_set_scan_parameters(dev, scan_type, interval, window,
                                           own_type, filter_policy, 10000))
            {
                return -1;
            }
        }
        else {
            return -1;
        }
    }

    uint8_t filter_duplicates = (flags & BLE_ADV_SCAN_FLAG_NO_DUPLICATES) ? 1 : 0;
    if (hci_le_set_scan_enable(dev, 1, filter_duplicates, 10000)) {
        return -1;
    }

    struct hci_filter hci_filter;
    hci_filter_clear(&hci_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &hci_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &hci_filter);

    if (setsockopt(dev, SOL_HCI, HCI_FILTER, &hci_filter, sizeof(hci_filter))) {
        return -1;
    }

    return 0;
}
/** @} */
