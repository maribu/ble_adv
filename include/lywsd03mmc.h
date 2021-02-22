/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LYWSD03MMC_H
#define LYWSD03MMC_H

#include "ble_adv.h"

#include <endian.h>
#include <stdint.h>

/**
 * @brief   Service Data UUID16 of a LYWSD03MMC measurement (with custom firmware)
 */
#define LYWSD03MMC_SERVICE_UUID16       0x181A

/**
 * @defgroup    lywsd03mmc  Trivial library to parse temperature / humidity data advertised by
 *                          Xiaomi LYWSD03MMC sensors running a custom firmware.
 *
 * @{
 * @brief   This header contains the public interface to the library
 * @file
 *
 * This software only works with the cheap Xiaomi LYWSD03MMC BLE temperature & humidity sensors and
 * only if the custom firmware at https://github.com/atc1441/ATC_MiThermometer or at
 * https://github.com/pvvx/ATC_MiThermometer is used.
 */

/**
 * @brief   Raw format of the service data send by the LYWSD03MMC custom firmware
 *
 * @note    This is a packed structure to mlywsd03mmch the exact memory layout and force the compiler
 *          to treat memory accesses as unaligned.
 */
struct lywsd03mmc_service_data {
    uint8_t addr[6];            /**< bluetooth address in correct byte order */
    int16_t temperature;        /**< Temperature in 0.1 °C, network byte order */
    uint8_t humidity;           /**< Relative humidity in % */
    uint8_t bat;                /**< Battery level in % */
    uint16_t bat_mv;            /**< Battery voltage in mV, network byte order */
    uint8_t frame_counter;      /**< Frame counter */
} __attribute__((packed));

/**
 * @brief   Parse measurement data
 */
struct lywsd03mmc_data{
    int16_t temperature;        /**< Temperature in 0.1 °C */
    uint16_t bat_mv;            /**< Battery voltage in mV */
    uint8_t humidity;           /**< Relative humidity in % */
    uint8_t bat;                /**< Battery level in % */
};

/**
 * @brief   Check if the given advertisement contains LYWSD03MMC measurement data
 *
 * @param[in]   adv     Advertisement to check
 *
 * @retval  1           The given advertisement contains LYWSD03MMC measurement data
 * @retval  0           The given advertisement contains ***NO*** LYWSD03MMC measurement data
 */
static inline int lywsd03mmc_is_match(const struct ble_adv *adv)
{
    if (adv && (adv->has & BLE_ADV_HAS_SERVICE_DATA)
        && (adv->service_uuid16 == LYWSD03MMC_SERVICE_UUID16)
        && (adv->service_data_len == sizeof(struct lywsd03mmc_service_data)))
    {
        return 1;
    }

    return 0;
}

/**
 * @brief   Extract the LYWSD03MMC measurement data of the given advertisement
 *
 * @param[out]  dest    Where to write the extracted data
 * @param[in]   adv     BLE advertisement to extract the measurement data from
 *
 * @pre     Calling @ref lywsd03mmc_is_match returns 1 on @p adv
 */
static inline void lywsd03mmc_parse(struct lywsd03mmc_data *dest, const struct ble_adv *adv)
{
    const struct lywsd03mmc_service_data *raw;
    raw = (const struct lywsd03mmc_service_data *)adv->service_data;
    dest->temperature = (int16_t)be16toh(raw->temperature);
    dest->bat_mv = be16toh(raw->bat_mv);
    dest->humidity = raw->humidity;
    dest->bat = raw->bat;
}

/** @} */
#endif /* LYWSD03MMC_H */
