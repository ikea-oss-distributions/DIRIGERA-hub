#!/bin/sh
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#

enable_boot_counter() {
    devmem2 0x5c00a154 w 0xb0010000
    touch "/usr/share/persist/.should_verify_system"
}

disable_boot_counter() {
    devmem2 0x5c00a154 w 0x00000000
    rm -f "/usr/share/persist/.should_verify_system"
}

usage() {
    echo "Usage: $(basename "$0") [enable|disable]"
}

test "$#" -ne 1 && { usage; exit $(( ! ! $# )); }
test "$1" = "help" -o "$1" = "-h" && { usage; exit 0; }
test "$1" = "enable" && { enable_boot_counter; sync; exit 0; }
test "$1" = "disable" && { disable_boot_counter; sync; exit 0; }
usage; exit 1
