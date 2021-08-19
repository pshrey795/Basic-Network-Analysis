#!/usr/bin/bash

#Determining maximum packet size
#Bounds for packet size
low=0
high=$((1<<16))  #Max value for packet size is 2^16-1
jump=$((($high)>>1))
command="ping $1 -w 1 -s "

while [ $jump -gt 0 ]
do
    next=$(($low + $jump))
    while [ $next -lt $high ]
    do
        $command$next
        if [ $? -eq 0 ]; then
            low=$next
            next=$(($low + $jump))
        else
            break
        fi
    done
    jump=$((($jump)>>1)) 
done

maxSize=$low

#Determining minimum TTL value
#Bounds for TTL
low=0
high=$((1<<8))  #Min value for TTL is 2^16-1
jump=$(($high>>1))
command="ping $1 -w 1 -t "

while [ $jump -gt 0 ]
do
    next=$(($low + $jump))
    while [ $next -lt $high ]
    do
        $command$next
        if [ $? -eq 0 ]; then
            break
        else
            low=$next
            next=$(($low + $jump))
        fi
    done
    jump=$(($jump>>1)) 
done

minSize=$(($low + 1))

echo ""
echo ""
echo ""
echo ""
echo ""
echo "The maximum packet size that can be sent is "$maxSize
echo "The minimum TTL value required is "$minSize