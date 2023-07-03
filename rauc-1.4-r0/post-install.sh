#!/bin/sh -x
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#
# shellcheck disable=SC2046

get_cur_rauc_slot() {
    set -- $(cat /proc/cmdline)
    for x; do
        case "$x" in
            rauc.slot=*)
                echo "${x#rauc.slot=}"
                ;;
        esac
    done
}

set_next_rauc_slot() {
    case "$(get_cur_rauc_slot)" in
        B)
            # current is B, switch to A
            mmc bootpart enable 1 1 /dev/mmcblk1
            ;;
        *)
            # current is A or unknown, switch to B
            mmc bootpart enable 2 1 /dev/mmcblk1
            ;;
    esac
}

clear_ota_ongoing() {
    devmem2 0x5c00a14c w $(( $(devmem2 0x5c00a14c | tail -n 1 | grep -oE '\S+$') & ~0x00000002 ))
    rm -f /usr/local/gw/.verity_ota_ongoing
}

clear_ota_ongoing
set_next_rauc_slot
/usr/sbin/boot-count.sh enable
