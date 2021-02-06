/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * @defgroup    scanner     Example program: Simple BLE scanner
 *
 * @{
 * @brief   This program demonstrates the usage of the ble_adv library
 * @file
 */
#include "ble_adv.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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

        printf("%s [%02X:%02X:%02X:%02X:%02X:%02X] RSSI: %u\n",
               adv.name, adv.addr[0], adv.addr[1], adv.addr[2], adv.addr[3], adv.addr[4],
               adv.addr[5], (unsigned)adv.rssi);

        if (adv.tx_power != INT8_MAX) {
            printf("   TX power: %d dBm\n", (int)adv.tx_power);
        }
        if (adv.uri_len) {
            printf("    URI: \"%s\"\n", adv.uri);
        }
        if (adv.has & BLE_ADV_HAS_UUID16) {
            printf("    UUID16: 0x%04X\n", adv.uuid16);
        }
        if (adv.has & BLE_ADV_HAS_UUID32) {
            printf("    UUID32: 0x%08X\n", adv.uuid16);
        }
        if (adv.has & BLE_ADV_HAS_UUID128) {
            printf("    UUID128: {0x%02X", adv.uuid128[0]);
            for (unsigned i = 1; i < sizeof(adv.uuid128); i++) {
                printf(", 0x%02X", adv.uuid128[i]);
            }
            puts("}");
        }
        if (adv.has & BLE_ADV_HAS_FLAGS) {
            puts("    Flags:");
            if (adv.flags & BLE_ADV_FLAGS_DISCO_LIMITED) {
                puts("        - LE Limited Discoverable Mode");
            }
            if (adv.flags & BLE_ADV_FLAGS_DISCO_GENERAL) {
                puts("        - LE General Discoverable Mode");
            }
            if (adv.flags & BLE_ADV_FLAGS_BLE_ONLY) {
                puts("        - Classic Bluetooth not supported");
            }
            if (adv.flags & BLE_ADV_FLAGS_BLE_MIXED_CONTROLLER) {
                puts("        - Simultaneous LE and BR/EDR to Same Device Capable (Controller)");
            }
            if (adv.flags & BLE_ADV_FLAGS_BLE_MIXED_HOST) {
                puts("        - Simultaneous LE and BR/EDR to Same Device Capable (Host)");
            }
        }
        if (adv.has & BLE_ADV_HAS_SERVICE_DATA) {
            printf("    Service 0x%04X: ", (unsigned)adv.service_uuid16);
            if (adv.service_data_len) {
                printf("{0x%02X", adv.service_data[0]);
                for (unsigned i = 1; i < adv.service_data_len; i++) {
                    printf(", 0x%02X", adv.service_data[i]);
                }
                puts("}");
            }
            else {
                puts("No data");
            }
        }
        if (adv.has & BLE_ADV_HAS_MS_DATA) {
            printf("    Manufacturer Specific Data 0x%04X: ", (unsigned)adv.ms_uuid16);
            if (adv.ms_data_len) {
                printf("{0x%02X", adv.ms_data[0]);
                for (unsigned i = 1; i < adv.ms_data_len; i++) {
                    printf(", 0x%02X", adv.ms_data[i]);
                }
                puts("}");
            }
            else {
                puts("No data");
            }
        }
    }
    return 0;
}

/** @} */
