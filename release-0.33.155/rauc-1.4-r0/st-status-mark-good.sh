#!/bin/sh
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#
#Disable bootcount
devmem2 0x5c00a154 w 0
if [ -f "/usr/share/persist/.should_verify_system" ]; then
    rm "/usr/share/persist/.should_verify_system"
fi

#Check verity
if [ -f "/usr/local/gw/.verity_ota_ongoing" ]; then
    echo "verity_ota_ongoing file found, setting ota ongoing in TAMP_BKP_19"
    devmem2 0x5c00a14c w 2
else
    devmem2 0x5c00a14c w 0
fi

/usr/bin/rauc status mark-good
