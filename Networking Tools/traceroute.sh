#!/usr/bin/bash

command="ping $1 -w 1 -t "
i=1

while [ $i -lt 20 ]
do
    $command$i
    i=$(($i + 1))
done

echo ""
echo ""
echo $1