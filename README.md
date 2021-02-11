adv_ble                                                                     {#mainpage}
=======

[TOC]

Usage in a Nutshell
===================

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
int main(void)
{
    int dev = ble_adv_open();
    if (dev == -1) {
        perror("ble_adv_open() failed");
        return EXIT_FAILURE;
    }

    if (ble_adv_scan(dev, BLE_ADV_SCAN_FLAG_ENABLED)) {
        perror("ble_adv_scan() failed");
        return EXIT_FAILURE;
    }

    while (1) {
        struct ble_adv advertisement;
        if (ble_adv_read(dev, &advertisement)) {
            perror("ble_adv_read() failed");
            /* disable scanning on failure */
            ble_adv_scan(dev, 0);
            return EXIT_FAILURE;
        }
        /* work on data */
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

What Does This Library Provide
==============================

This tiny C library that provides a convenience wrapper over BlueZ to allow scanning for Bluetooth
Low Energy (BLE) Advertisements with as little effort as possible. In addition, it parses the most
important entries of BLE advertisements into a structure, so that users don't need to worry about
the encoding.

What Does This Library Not Provide
==================================

Only a subset of the specified
[EIR data types](https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/)
are implemented. It should however be relatively straight forward to extend the `parse_eir()`
function to support additional fields. Search for the "Supplement to the Bluetooth Core
Specification" and jump to "Part A: Data Types Specification".

It is worth noting that only the service data with a 16 bit UUID are supported, while also 32 and
128 bit flavors are specified.

Motivation
==========

Even as a senior (*ahem* *ahem*) C developer I found it surprisingly hard to get started with BlueZ.
Due to the lack of documentation, resources, and even third party resources on this matter, I ended
up studying the source code of BlueZ to be able to interface with it. The goal of this library is
to get C developers to receive BLE advertisements with as little pain in the ass as possible.

Permissions and Security
========================

Enabling/disabling scanning for BLE advertisements and configuring the scan parameters cannot be
done as regular user. Still, running programs written in C (and, hence, without any guarantees
on memory safety) which parse (somewhat) complex data formats of messages received from untrusted
sources as `root` is ranked pretty high on the list of things I'd personally avoid doing. Luckily,
the situation is not as bad as it initially sounds:

1.  Only two capabilities are actually needed to, not full `root` access. Just run
    <code>sudo setcap 'cap_net_raw,cap_net_admin+eip' &lt;program&gt;</code> on your executable to
    get it running.
2.  You could implement your program in a way that it drops its capabilities right after it enabled
    scanning for BLE advertisements but before reading the first advertisements. E.g.
    with [libcap-ng](https://people.redhat.com/sgrubb/libcap-ng/) this is straight forward.
    However, your program won't be able to disable scanning after it has completed its task.
3.  You could fork in your program and let e.g. the parent process keep the capabilities, while the
    child processg gives them up. The parent will only enable and disable BLE scanning, but leave
    the parsing of the child process. This way, the process doing the risky part runs without extra
    capabilities.
