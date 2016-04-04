#!/bin/bash
ba-elf-readelf -s slip-radio.jn516x-neeo | grep OBJECT | grep 0400 | awk '{ sum+=$3} END {printf "RAM usage: %u bytes\n", sum}'
