#!/usr/bin/env bash
if [ $# -ne 1 ]; then
    echo "Usage: ./init_pi.sh ip_address" 
    echo "e.g.: ./init_pi.sh 192.168.0.123"
    exit 1
fi
scp neeo/raspberrypi/id_pi.pub root@$1:~/.ssh/authorized_keys 
scp -i neeo/raspberrypi/id_pi -r neeo/raspberrypi/jenprog/ root@$1:~/jenprog
