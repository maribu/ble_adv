/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef BLE_ADV_H
#define BLE_ADV_H

/**
 * @defgroup    ble_adv     Convenience C library to easily receive BLE advertisements using bluez
 *
 * @{
 * @brief   This header contains the public interface to the library
 * @file
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/**
 * @name    Flags used in the second argument of @ref ble_adv_scan
 * @{
 */
#define BLE_ADV_SCAN_FLAG_ENABLED           0x01    /**< Enable scanning */
#define BLE_ADV_SCAN_FLAG_NO_DUPLICATES     0x02    /**< Filter duplicates */
#define BLE_ADV_SCAN_FLAG_PASSIVE           0x04    /**< Passive scanning */
#define BLE_ADV_SCAN_FLAG_PUBLIC_ADDR       0x08    /**< Use public address for active scan */
/** @} */

/**
 * @name    Flags used in @ref ble_adv::has
 * @{
 */
#define BLE_ADV_HAS_UUID16                  0x01    /**< Advertisement contained an UUID16 */
#define BLE_ADV_HAS_UUID32                  0x02    /**< Advertisement contained an UUID32 */
#define BLE_ADV_HAS_UUID128                 0x04    /**< Advertisement contained an UUID128 */
#define BLE_ADV_HAS_SERVICE_DATA            0x08    /**< Advertisement contained service data */
#define BLE_ADV_HAS_MS_DATA                 0x10    /**< Advertisement contained manufacturer
                                                         specific data */
#define BLE_ADV_HAS_FLAGS                   0x20    /**< Advertisement contained flags */
/** @} */


/**
 * @name    Flags used in @ref ble_adv::flags
 * @{
 */
#define BLE_ADV_FLAGS_DISCO_LIMITED         0x01    /**< LE Limited Discoverable Mode  */
#define BLE_ADV_FLAGS_DISCO_GENERAL         0x02    /**< LE General Discoverable Mode */
#define BLE_ADV_FLAGS_BLE_ONLY              0x04    /**< Classic Bluetooth not supported */
#define BLE_ADV_FLAGS_BLE_MIXED_CONTROLLER  0x08    /**< Simultaneous LE and BR/EDR to Same Device
                                                         Capable (Controller) */
#define BLE_ADV_FLAGS_BLE_MIXED_HOST        0x10    /**< Simultaneous LE and BR/EDR to Same Device
                                                         Capable (Host) */
/** @} */

/**
 * @brief   Structure holding parsed info about a received advertisement
 */
struct ble_adv {
    uint8_t uuid128[16];        /**< UUID128 if present (little endian), check @ref ble_adv::has */
    uint32_t uuid32;            /**< UUID32 if present, check @ref ble_adv::has */
    uint16_t uuid16;            /**< UUID16 if present, check @ref ble_adv::has */
    uint16_t service_uuid16;    /**< UUID16 of the service data, check @ref ble_adv::has */
    uint16_t ms_uuid16;         /**< UUID16 of the manufacturer specific data,
                                     check @ref ble_adv::has */
    uint8_t addr[6];            /**< Address of the sender in corrected byte order */
    /**
     * @brief   Zero-terminated name of the sender or `"<unknown>"`
     *
     * @note    @ref ble_adv::name_len will have a value of 0 if the name is unknown,
     *          even though the zero terminated string `"<unknown>"` will be stored here in
     *          this case.
     */
    char name[29];
    /**
     * @brief   Length of the @ref ble_adv::name without terminating zero byte
     *
     * @note    This will be zero in case the name is unknown.
     */
    uint8_t name_len;
    uint8_t service_data[27];   /**< Service data */
    uint8_t service_data_len;   /**< Length of @ref ble_adv::service_data in bytes */
    uint8_t ms_data[27];        /**< Manufacturer specific data */
    uint8_t ms_data_len;        /**< Length of @ref ble_adv::ms_data in bytes */
    char uri[30];               /**< URI which is advertised (zero terminated) */
    uint8_t uri_len;            /**< Length of @ref ble_adv::uri in bytes */
    uint8_t flags;              /**< Flags carried by the advertisement, check @ref ble_adv::has */
    uint8_t rssi;               /**< Received signal strength indicator */
    int8_t tx_power;            /**< TX-power the sender claimed it used or INT8_MAX */
    uint8_t has;                /**< Flags used to indicate availability of fields */
};

/**
 * @brief   Convenience function to open a descriptor of the first HCI interface
 * @return  The opened device descriptor
 * @retval  -1                  Failed to open descriptor and errno set
 *                              to indicate the cause
 */
static inline int ble_adv_open(void)
{
    return hci_open_dev(hci_get_route(NULL));
}

/**
 * @brief   Enable/disable BLE scanning and set appropriate filters
 *
 * @param[in]       dev         Descriptor of the HCI interface
 * @param[in]       flags       Flags describing the state to set, e.g. see
 *                              @ref BLE_ADV_SCAN_FLAG_ENABLED
 * @retval  0                   Success
 * @retval -1                   Failure and errno set to indicate the cause
 */
int ble_adv_scan(int dev, unsigned flags);

/**
 * @brief   Read a single BLE advertisement from the HCI descriptor
 *
 * @param[in]       dev         Descriptor of the HCI interfaces
 * @param[out]      dest        Structure to write the received BLE advertisement to
 *
 * @retval  0                   Success
 * @retval  -1                  Failure and errno is set to indicate the cause
 *
 * @note    If @p dev is in non-blocking mode, expect "failures" with
 *          `(errno == EWOULDBLOCK) || (errno == EAGAIN)`. This function will internally
 *          retry on `errno == EINTR` until it either succeeds are fails for a different
 *          reason.
 *
 * @pre     Scanning for BLE has been enabled via @ref ble_adv_scan first
 */
int ble_adv_read(int dev, struct ble_adv *dest);

/** @} */
#endif /* BLE_ADV_H */
