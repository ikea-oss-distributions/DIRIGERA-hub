#!/bin/sh
# Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of © Inter IKEA Systems B.V.
#
# shellcheck disable=SC2046

for file in "${RAUC_BUNDLE_MOUNT_POINT}"/*; do
    if [ -f "${file}" ]; then
        if echo "${file##*/}" | grep -q "^fip-"; then
            echo "Updating to OpenSTLinux 3.x"
            "${0%/*}/post-install-3.x.sh"
            exit $?
        fi
    fi
done

echo "Updating to OpenSTLinux 2.x"
"${0%/*}/post-install-2.x.sh"
exit $?
