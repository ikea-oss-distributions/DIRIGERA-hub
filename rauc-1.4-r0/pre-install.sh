#!/bin/sh -x
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#
# Remove write protection on boot partitions
echo "0" > /sys/class/block/mmcblk1boot0/force_ro
echo "0" > /sys/class/block/mmcblk1boot1/force_ro

BOOT_COUNTER_STATUS="$(devmem2 0x5c00a154 | grep Read | awk '{print $6}')"

if [ "${BOOT_COUNTER_STATUS}" != "0x00000000" ]
then
    logger -s -p 3 "Error boot counter not disabled when trying to install fw"
    exit 1
fi

for file in "${RAUC_BUNDLE_MOUNT_POINT}"/*.stm32; do
    if [ -f "${file}" ]; then
        if ! verifybl "${file}" ; then
            logger -s -p 3 "Failed to verify signature"
            exit 1
        fi
    fi
done

# Set OTA ongoing flag in verity TAMP
devmem2 0x5c00a14c w 0x00000002
# Create file in userfs in case power is lost and TAMP is cleared.
mkdir -p /usr/local/gw
touch /usr/local/gw/.verity_ota_ongoing
