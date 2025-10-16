#!/usr/bin/env bash
set -euo pipefail

MOD=TimerDriver
KO=./${MOD}.ko
DEV=/dev/${MOD}
MAJOR=415
MINOR=0

#run this in case you're having trouble and make sure
#you're in the rigth kernel
uname -r

if lsmod | awk '{print $1}' | grep -qx "$MOD";then
    sudo rmmod "$MOD"
fi

#inserts the module
sudo insmod "$KO"

#Create device node if missing
if [ ! -e "$DEV" ]; then
    sudo mknod "$DEV" c "$MAJOR" "$MINOR"
fi

sudo chmod 666 "$DEV"

dmesg | tail -n 20
echo "Ready: $DEV"

