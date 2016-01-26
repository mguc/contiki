#!/usr/bin/env bash
if [ $# -ne 2 ]; then
    echo "Usage: ./remote_flash.sh path/to/binary ip_address" 
    echo "e.g.: ./remote_flash.sh path/to/jn5168.bin 192.168.0.123"
    exit 1
fi
echo "Remote Flashing of ${1} on address ${2}" 

scp -i id_pi $1 pi@$2:jenprog
filename=$(basename $1)
ssh -i id_pi pi@$2 "cd jenprog && ./update_firmware.sh ${filename}"
