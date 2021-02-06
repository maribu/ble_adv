/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * @defgroup    atc_dumper  Example program: Dump data from ATC temperature and humidity sensors
 *
 * @{
 * @brief   This program demonstrates the usage of the ble_adv library.
 * @file
 *
 * This program dumps the data received from cheap BLE temperature and humidity sensors, provided
 * the custom firmware at https://github.com/atc1441/ATC_MiThermometer is installed.
 */
#include "ble_adv.h"

#include <endian.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief   Raw format of the service data send by the ATC custom firmware
 *
 * @note    This is a packed structure to match the exact memory layout and force the compiler
 *          to treat memory accesses as unaligned.
 */
struct atc_service_data {
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
struct measurement {
    int16_t temperature;        /**< Temperature in 0.1 °C */
    uint16_t bat_mv;            /**< Battery voltage in mV */
    uint8_t humidity;           /**< Relative humidity in % */
    uint8_t bat;                /**< Battery level in % */
};

static int dev;

static void handle_exit(int signal)
{
    (void)signal;
    /* stop BLE scanning on exit */
    ble_adv_scan(dev, 0);
    exit(EXIT_SUCCESS);
}

static struct sigaction exit_handler = {
    .sa_handler = handle_exit,
};

static void parse_measurement(struct measurement *dest, const uint8_t *data)
{
    const struct atc_service_data *raw = (const struct atc_service_data *)data;
    dest->temperature = be16toh(raw->temperature);
    dest->bat_mv = be16toh(raw->bat_mv);
    dest->humidity = raw->humidity;
    dest->bat = raw->bat;
}

int main(int argc, const char **argv)
{
    (void)argc;
    dev = ble_adv_open();
    if (dev < 0) {
        perror("ble_adv_open()");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &exit_handler, NULL) || sigaction(SIGTERM, &exit_handler, NULL)) {
        puts("WARNING: Couldn't register exit handler to disable scanning on exit");
    }

    if (ble_adv_scan(dev, BLE_ADV_SCAN_FLAG_ENABLED)) {
        int err = errno;
        perror("ble_adv_scan() failed");
        if (err == EPERM) {
            printf("Try running \"sudo setcap 'cap_net_raw,cap_net_admin+eip' %s\"\n", argv[0]);
            /* theoretically, puts() could have overwritten value of errno */
        }
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct ble_adv adv;
        if (ble_adv_read(dev, &adv)) {
            perror("reading advertisement");
            ble_adv_scan(dev, 0);
            exit(EXIT_FAILURE);
        }

        if ((adv.has & BLE_ADV_HAS_SERVICE_DATA) && (adv.service_uuid16 = 0x181a)
            && (adv.service_data_len == sizeof(struct atc_service_data))) {
            printf("%s [%02X:%02X:%02X:%02X:%02X:%02X] RSSI: %u\n",
                   adv.name, adv.addr[0], adv.addr[1], adv.addr[2], adv.addr[3], adv.addr[4],
                   adv.addr[5], (unsigned)adv.rssi);
            struct measurement data;
            parse_measurement(&data, adv.service_data);
            int temp_int; unsigned temp_rem;
            if (data.temperature >= 0) {
                temp_int = data.temperature / 10;
                temp_rem = data.temperature % 10;
            }
            else {
                temp_int = (data.temperature + 9) / 10;
                temp_rem = (-data.temperature) % 10;
            }
            printf("temperature = %d.%u °C, humidity = %u %%, battery = %u %% (%u mV)\n",
                   temp_int, temp_rem, data.humidity, data.bat, data.bat_mv);
        }
    }
    return 0;
}

/** @} */
