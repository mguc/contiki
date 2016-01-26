#!/usr/bin/env bash
if [ $# -ne 1 ]; then
    echo "Usage: ./init_pi.sh ip_address" 
    echo "e.g.: ./init_pi.sh 192.168.0.123"
    exit 1
fi
scp id_pi.pub pi@$1:~/.ssh/authorized_keys 
scp -i id_pi -r jenprog/ pi@$1:jenprog
