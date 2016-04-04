#!/bin/bash

set -e

HERE=$(dirname $0)

echo ----- DOCKER build TR2 $(pwd)
pushd build/docker/builder/
docker build -t tr2contiki .
popd

echo ----- DOCKER run TR2 $(pwd)
rm -fr ./dist/border-router.native && mkdir -p dist
echo mount $(pwd) as /source

# build cp6 NBR binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/native-border-router && TARGET=native CROSS_COMPILE=arm-linux-gnueabihf- make distclean all && cp ./border-router.native /out'
