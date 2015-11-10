#!/bin/bash

# Set compiler path
export PATH="/projects/gnutools/arm-eabi/bin:home/ws/workspace/gnutools/arm-eabi/bin:$PATH"   # modify this

# Set compiler options
OPT="-Wall -Wno-psabi -Wpointer-arith -Wno-write-strings -mthumb -g -O2 -ffunction-sections -fdata-sections -fno-exceptions -nostdlib -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16"

# Set path to eCos kernel
KPATH="eCos_headers"

CPP_FILES="src/*.cpp Third-Party/tinyxml2/*.cpp" 
C_FILES="Third-Party/libpng-1.5.14/*.c src/*.c"

#Clear
rm -r o_files libgui.a 2> /dev/null

#Add files
arm-eabi-g++ -g -Iinc/ -IThird-Party/tinyxml2 -IThird-Party/libpng-1.5.14 -I${KPATH}/include -c ${CPP_FILES} ${OPT}
arm-eabi-gcc -Iinc/ -IThird-Party/tinyxml2 -IThird-Party/libpng-1.5.14 -I${KPATH}/include -c ${C_FILES} ${OPT}

#Create library
arm-eabi-ar rcs libgui.a *.o

#Move files
mkdir o_files

mv *.o o_files/
