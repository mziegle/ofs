#!/bin/sh
module="ofs.ko"
device="openFileSearchDev"
owner="osboxes"
group="users"

deviceinfo=`cat /proc/devices | grep ${device}`

if [ "$deviceinfo" != "" ]; then
  rmmod $module
  echo "old $module module removed"
fi

insmod $module || exit 1
echo "new $module module inserted"

rm -f ${device}
echo "old ${device} device removed"

major=`cat /proc/devices | grep ${device} | awk '{print $1}'`

mknod ${device} c $major 0
echo "new ${device} device with major number $major created"

chown $owner ${device}
chgrp $group ${device}



