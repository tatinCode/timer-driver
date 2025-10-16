#!usr/bin/env bash
set -euo pipefail

MOD=TimerDriver
DEV=/dev/${MOD}

#if the device node exists, remove it
if [ -e "$DEV" ]; then
    echo "Removing device node $MOD..."
    sudo rm -f "$DEV"
fi


if lsmod | awk '{print $1}' | grep -qx "$MOD";then
    echo "Unloading kernel module  $MOD..."
    sudo rmmod "$MOD"
else
    echo "Module $MOD is not loaded"
fi

echo "Cleanup complete"
