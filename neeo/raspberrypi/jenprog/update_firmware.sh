#!/usr/bin/env bash
if [ $# -ne 1 ]; then
  echo "Usage: ./update_firmware.sh path/to/binary"
  echo "e.g.: ./update_firmware.sh jn5168.bin"
  exit 1
fi
./jenprog -c serial -t /dev/ttyAMA0 $1
./jn_reset.sh
