#!/bin/bash

set -e

HERE=$(dirname $0)

echo ----- DOCKER build TR2 $(pwd)
pushd build/docker/builder/
docker build -t tr2contiki .
popd

echo ----- DOCKER run TR2 $(pwd)
rm -fr ./dist/jn516x-tr2.bin && mkdir -p dist
echo mount $(pwd) as /source

# build tr2 JN5168 binary
docker run --rm -a stdin -a stdout -a stderr -v $(pwd)/dist:/out -v $(pwd):/source tr2contiki bash -c 'cd /source/neeo/comm-tests && PATH=$PATH:/builder/bin WITH_FEM=1 TARGET=jn516x-neeo make all'
