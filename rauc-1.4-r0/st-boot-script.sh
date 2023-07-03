#/bin/bash
# To disable bootcount, both active_index and previous_active_index have to be equal

arg1=$1
arg2=$2

if [ "$arg1" = "get-primary" ]
then
    cat /proc/cmdline | grep "rw rauc.slot=A"
    if test $? == 0
    then
        /usr/lib/fwu/update_metadata.sh 0 0
        echo "A"
    else
        /usr/lib/fwu/update_metadata.sh 1 1
        echo "B"
    fi
    exit 0
fi

# When boot succeeded, st-status-mark-good.sh calls rauc status mark-good which
# enters in this function.
if [ "$arg1" = "get-state" ]
then
    echo "good"
    exit 0
fi
