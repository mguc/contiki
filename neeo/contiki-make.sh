#!/bin/bash

#start with building contiki
echo "Building Contiki..."

pushd ../contiki-jn5168/neeo/tr2-com-firmware
    make WITH_FEM=1 && make deploy DESTINATION_FILE=../../../neeo/romfs/jn.bin
    if [ $? -ne 0 ] ; then
        echo -e "\e[1;91mBuilding Contiki failed.\e[21;39m!"
        exit
    fi
    echo -e "\e[1;32mBuilding Contiki finished.\e[21;39m!"
popd
