#!/bin/sh
cd romfs
../bootloader_build/install/bin/mk_romfs . ../include/romfs.img
cd ../include
python generate.py romfs.img romfs romfs_image.h
cd ..
