#!/bin/bash

set -e

HERE=$(dirname $0)

echo ----- DOCKER build TR2 $(pwd)
pushd build/docker/builder/
docker build -t tr2contiki .
popd

echo ----- DOCKER run TR2 $(pwd)
rm -fr ./dist/jn516x-cp6.bin && mkdir -p dist
echo mount $(pwd) as /source

# build cp6 JN5168 binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/slip-radio && PATH=$PATH:/builder/bin MODULE=M05 TARGET=jn516x-temp make distclean all && cp ./slip-radio.jn516x-temp.bin /out/jn516x-cp6.bin'
