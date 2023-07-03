#!/bin/sh
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#
# This script verifies that gateway.target is running then notifies systemd that this service is ready.
# Before exit it validates that the system is running as a whole, when system is running $2 is touched.

set -exuC -o
PASSED="OTA Validation passed"
FAILED="OTA validation failed"
TIME_BEFORE_REBOOT=10
if [ -f "/etc/devimage.flg" ]; then
    TIME_BEFORE_REBOOT=900
fi

system_is_started() {
    STATUS=$(systemctl is-system-running)
    test "${STATUS}" = "running"
}
system_is_degraded() {
    STATUS=$(systemctl is-system-running)
    test "${STATUS}" = "degraded"
}

gateway_is_active() {
    systemctl is-active --quiet gateway.target
}

teardown_before_reboot() {
    systemctl --state=failed | tee /dev/kmsg
    sleep "${TIME_BEFORE_REBOOT}"
}

BOOT_COUNTER_STATUS="$(devmem2 0x5c00a154 | grep Read | awk '{print $6}')"
if [ "${BOOT_COUNTER_STATUS}" != "0x00000000" ]
then
    CURRENT="0"
    END="900"
    while [ "${CURRENT}" -lt "${END}" ]; do
        if system_is_started; then
            if gateway_is_active; then
                echo "${PASSED}"
                exit 0
            else
                logger -s -p 3 "${FAILED}: gateway.target was not reached waiting ${TIME_BEFORE_REBOOT} seconds before reboot"
                teardown_before_reboot
                exit 1
            fi
        elif system_is_degraded; then
            logger -s -p 3 "${FAILED}: system is degraded waiting ${TIME_BEFORE_REBOOT} seconds before reboot"
            teardown_before_reboot
            exit 1
        fi
        sleep 1
        CURRENT=$(( CURRENT+1 ))
    done

    logger -s -p 3 "${FAILED}: Validation timeout reach ${END} seconds"
    exit 1
fi
echo "${PASSED}: Boot counter already disabled, will not verify system"
exit 0
