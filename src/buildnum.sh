#!/bin/sh
#
#	Shell script to update the build number
#
PATH=/bin:/usr/bin
#
if [ ! -f buildnum.data ]; then
    touch buildnum.data
    echo 1 > buildnum.data
    echo 1
else
    bnum=`awk '{ print $1 + 1 }' < buildnum.data`
    echo $bnum > buildnum.data
    echo $bnum
fi
