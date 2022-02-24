#!/bin/sh

ENVFILE=${UMRPATH}/saved_env

# Siphon a few things from nickel's env (namely, stuff exported by rcS *after* on-animator.sh has been launched)...
export $(grep -s -E -e '^(DBUS_SESSION_BUS_ADDRESS|NICKEL_HOME|WIFI_MODULE|LANG|INTERFACE)=' "/proc/$(pidof -s nickel)/environ")
export DBUS_SESSION_BUS_ADDRESS NICKEL_HOME WIFI_MODULE LANG INTERFACE

export ORIG_FB_ROTA="$(cat /sys/class/graphics/fb0/rotate)"

sync

killall -q -TERM nickel hindenburg sickel fickel adobehost dhcpcd-dbus dhcpcd fmon kfmon 2>/dev/null

