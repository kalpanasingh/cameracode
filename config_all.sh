#!/bin/bash
#shell script to set configuration option for all attached cameras,
#call as follows ./config_all.sh [configuration option] [value]

# Map Ports function.  Builds an array PORT[n] containing a series of
#active USB ports and the PORTS variable containing the number of ports active
mapports(){
    portstring=$(gphoto2 --auto-detect |egrep -o "usb:...,...")
    ports=$(echo $portstring | wc -w)
    PORT=( $(gphoto2 --auto-detect |egrep -o "usb:...,...") )
}

mapports
for((j=0;j<ports;j++));
do
gphoto2 --port=${PORT[j]} --set-config $1=$2;
done
