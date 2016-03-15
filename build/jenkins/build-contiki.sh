#!/bin/bash

set -e

HERE=$(dirname $0)

echo ----- DOCKER build TR2 $(pwd)
pushd build/docker/builder/
docker build -t tr2contiki .
popd

echo ----- DOCKER run TR2 $(pwd)
rm -fr ./dist/* && mkdir -p dist
echo mount $(pwd) as /source

# build tr2 JN5168 binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/tr2-com-firmware && PATH=$PATH:/builder/bin MODULE=M05 TARGET=jn516x-temp make distclean all && cp ./tr2-com-firmware.jn516x-temp.bin /out/jn516x-tr2.bin && CONTIKI_VERSION=$(grep -h "#define FW_MAJOR_VERSION " tr2-com-firmware.c | tr -s " " | cut -d\  -f 3- | tr -d \") && printf -v v "%08x" $CONTIKI_VERSION && echo ${v:6:2}${v:4:2}${v:2:2}${v:0:2} | while read -N2 code; do printf "\x$code"; done | dd conv=notrunc of=/out/jn516x-tr2.bin bs=1 count=4'

# build cp6 JN5168 binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/slip-radio && PATH=$PATH:/builder/bin MODULE=M05 TARGET=jn516x-temp make distclean all && cp ./slip-radio.jn516x-temp.bin /out/jn516x-cp6.bin'

# build cp6 NBR binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/native-border-router && TARGET=native CROSS_COMPILE=arm-linux-gnueabihf- make distclean all && cp ./border-router.native /out'

# build rpi NBR binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/native-border-router && TARGET=native CROSS_COMPILE=/builder/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf- make distclean all && cp ./border-router.native /out/border-router.rpi'
