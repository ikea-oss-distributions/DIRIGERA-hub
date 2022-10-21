#!/bin/sh -ex
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#

if [ "$1" = "get-primary" ]; then
    awk -F' ' '/rauc.slot/ {print gensub(".*rauc.slot=(\\S*).*", "\\1", 1)}' /proc/cmdline
elif [ "$1" = "get-state" ]; then
    echo "good"
fi

exit 0
