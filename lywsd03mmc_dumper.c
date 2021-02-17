/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * @defgroup    lywsd03mmc_dumper   Example program: Dump data from LYWSD03MMC temperature and
 *                                  humidity sensors
 *
 * @{
 * @brief   This program demonstrates the usage of the ble_adv library.
 * @file
 *
 * This program dumps the data received from cheap LYWSD03MMC BLE temperature and humidity sensors,
 * provided the [this](https://github.com/atc1441/ATC_MiThermometer) or
 * [this](https://github.com/pvvx/ATC_MiThermometer) custom firmware is used.
 */
#include "ble_adv.h"
#include "lywsd03mmc.h"

#include <endian.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static int dev;

static void __attribute__((noreturn)) handle_exit(int signal)
{
    (void)signal;
    /* stop BLE scanning on exit */
    ble_adv_scan(dev, 0);
    exit(EXIT_SUCCESS);
}

static struct sigaction exit_handler = {
    .sa_handler = handle_exit,
};

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
            && (adv.service_data_len == sizeof(struct lywsd03mmc_service_data))) {
            printf("%s [%02X:%02X:%02X:%02X:%02X:%02X] RSSI: %u\n",
                   adv.name, adv.addr[0], adv.addr[1], adv.addr[2], adv.addr[3], adv.addr[4],
                   adv.addr[5], (unsigned)adv.rssi);
            struct lywsd03mmc_data data;
            lywsd03mmc_parse(&data, adv.service_data);
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
}

/** @} */
