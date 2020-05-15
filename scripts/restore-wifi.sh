#!/bin/sh

export PATH="${PATH}:/sbin"

PLATFORM=freescale
if [ `dd if=/dev/mmcblk0 bs=512 skip=1024 count=1  2>/dev/null | grep -c "HW CONFIG" ` == 1 ]; then
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

RestoreWifi() {
    #./enable-wifi.sh
    lsmod | grep -q sdio_wifi_pwr || insmod "/drivers/${PLATFORM}/wifi/sdio_wifi_pwr.ko"
    # Moar sleep!
    usleep 250000
    lsmod | grep -q "${WIFI_MODULE}" || insmod "/drivers/${PLATFORM}/wifi/${WIFI_MODULE}.ko"
    # Race-y as hell, don't try to optimize this!
    sleep 1

    ifconfig "${INTERFACE}" up
    [ "$WIFI_MODULE" != "8189fs" ] && [ "${WIFI_MODULE}" != "8192es" ] && wlarm_le -i "${INTERFACE}" up

    pidof wpa_supplicant >/dev/null \
        || env -u LD_LIBRARY_PATH \
            wpa_supplicant -D wext -s -i "${INTERFACE}" -O /var/run/wpa_supplicant -c /etc/wpa_supplicant/wpa_supplicant.conf -B


    # Much like we do in the UI, ensure wpa_supplicant did its job properly, first.
    # Pilfered from https://github.com/shermp/Kobo-UNCaGED/pull/21 ;)
    wpac_timeout=0
    while ! wpa_cli status | grep -q "wpa_state=COMPLETED"; do
        # If wpa_supplicant hasn't connected within 10 seconds, assume it never will, and tear down WiFi
        if [ ${wpac_timeout} -ge 40 ]; then
            echo "[$(date)] restore-wifi-async.sh: Failed to connect to preferred AP!"

            #./disable-wifi.sh
            killall udhcpc default.script wpa_supplicant 2>/dev/null

            [ "${WIFI_MODULE}" != "8189fs" ] && [ "${WIFI_MODULE}" != "8192es" ] && wlarm_le -i "${INTERFACE}" down
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

            return 1
        fi
        usleep 250000
        wpac_timeout=$((wpac_timeout + 1))
    done

    #./obtain-ip.sh

    # Release IP and shutdown udhcpc.
    pkill -9 -f '/bin/sh /etc/udhcpc.d/default.script'
    ifconfig "${INTERFACE}" 0.0.0.0

    # Use udhcpc to obtain IP.
    env -u LD_LIBRARY_PATH udhcpc -S -i "${INTERFACE}" -s /etc/udhcpc.d/default.script -t15 -T10 -A3 -b -q
}

RestoreWifi 2>&1 >/dev/null
