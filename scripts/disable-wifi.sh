#!/bin/sh

export PATH="${PATH}:/sbin"

PLATFORM=freescale
if [ `dd if=/dev/mmcblk0 bs=512 skip=1024 count=1 2>/dev/null | grep -c "HW CONFIG"` == 1 ]; then
        CPU=`ntx_hwconfig -s -p /dev/mmcblk0 CPU 2>/dev/null`
        PLATFORM=$CPU-ntx
        WIFI=`ntx_hwconfig -s -p /dev/mmcblk0 Wifi 2>/dev/null`
fi

INTERFACE=wlan0
WIFI_MODULE=ar6000
if [ $PLATFORM != freescale ]; then
        INTERFACE=eth0
        WIFI_MODULE=dhd
        if [ x$WIFI == "xRTL8189" ]; then
            WIFI_MODULE=8189fs
        fi
fi

# Disable wifi, and remove all modules.

killall udhcpc default.script wpa_supplicant 2>/dev/null

[ "${WIFI_MODULE}" != "8189fs" ] && [ "${WIFI_MODULE}" != "8192es" ] && [ "$WIFI_MODULE" != 8821cs ] && wlarm_le -i "${INTERFACE}" down
ifconfig "${INTERFACE}" down

# Some sleep in between may avoid system getting hung
# (we test if a module is actually loaded to avoid unneeded sleeps)
if lsmod | grep -q "${WIFI_MODULE}"; then
    usleep 250000
    rmmod "${WIFI_MODULE}"
fi
if lsmod | grep -q sdio_wifi_pwr; then
    usleep 250000
    rmmod sdio_wifi_pwr
fi

# Release IP and shutdown udhcpc.
pkill -9 -f '/bin/sh /etc/udhcpc.d/default.script'
ifconfig "${INTERFACE}" 0.0.0.0
