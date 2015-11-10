#!/bin/bash
# check for ecosconfig
command -v ecosconfig >/dev/null 2>&1 || { echo >&2 "FATAL: ecosconfig is not installed in the system. Put it in /usr/local/bin or an equivalent location."; exit 1; }

function get_option {
    VALUE=`echo $1 | sed 's/[-a-zA-Z0-9]*=//'`
}

function ecc_get_value {
    echo `cat $ECC | grep $1 -A 100 | grep user_value | head -1 | sed 's,user_value,|,g' | cut -f 2 -d '|' | tr -d '"\r\n'`
}


function usage {
        echo "Usage: `basename $0` --config=<config-name> (--output-filename=<filename>|--tests|--rebuild)"
        echo "       Use a config with FILES='' to just generate a kernel"
}

TESTS=false
REBUILD=false
OMIT_CONTIKI=false

for i in "$@"
do
get_option $i
case $i in
    -c=*|--config=*)
        CONFIG=$VALUE
        ;;
    -o=*|--output-filename=*)
        OUTPUT_FILENAME=$VALUE    
        ;;
    -t|--tests)
        TESTS=true
        ;;
    -r|--rebuild)
        REBUILD=true
	;;
    *)
        echo "FATAL: You have provided an unknown option: $i."
	usage
        exit 1
esac
done


if [ -z $CONFIG ]
then
        FIRST_FILE=`ls *.config | head -1`
        if [ -e "$FIRST_FILE" ]
        then
                CONFIG=$FIRST_FILE
        fi
fi

if [ -z $CONFIG ]
then
	echo "FATAL: No config file provided."
	usage
	exit 1
fi

CONFIG="${CONFIG%.*}"

CONFIG_FILE=$CONFIG.config

if [ ! -e $CONFIG_FILE ]
then
	echo "FATAL: The config file \"$CONFIG_FILE\" does not exist."
	exit 1
fi

echo "Using file: $CONFIG_FILE"

TPATH_FILE=$CONFIG.tpath

if [ -e $TPATH_FILE ]
then
	# include the toolchain path file, if it exits
	export PATH="`cat $TPATH_FILE`:$PATH"
fi

# include config file to set appropriate variables
. ./$CONFIG_FILE

ECOS_REPOSITORY=`readlink -f $ECOS_REPOSITORY`
ECC=`readlink -f $ECC`

if [ -z $ECOS_REPOSITORY ] || [ -z $ECC ]
then
	echo "FATAL: The variables \"ECOS_REPOSITORY\" AND \"ECC\" have not been set properly."
	exit 1
fi

GCC_PREFIX=`ecc_get_value CYGBLD_GLOBAL_COMMAND_PREFIX`-
CFLAGS=`ecc_get_value CYGBLD_GLOBAL_CFLAGS`

GCC=${GCC_PREFIX}gcc

# check for compiler
command -v $GCC >/dev/null 2>&1 || { echo >&2 "FATAL: The required compiler ($GCC) does not exist in your PATH. Did you forget an appropriate $CONFIG.tpath file?"; exit 1; }

mkdir -p $CONFIG\_build
if $REBUILD ; then
  rm -rf $CONFIG\_build/*
fi
cd $CONFIG\_build
KPATH=`pwd`

ecosconfig --config=$ECC tree
if $TESTS ; then
   make tests
   exit
else
   make
fi

cd ..

if [ ! -z "$FILES" ]
then
   if [ -z $OUTPUT_FILENAME ] ; then
      OUTPUT_FILENAME=$CONFIG
   fi

COMMIT=`git log | head -1 | cut -f 2 -d ' ' | cut -b 1-8`
echo "#define GIT_COMMIT \"${COMMIT}\"" > include/version.h
echo >> include/version.h

echo "Generating fonts..."

rm -rf romfs/fonts 2> /dev/null
mkdir romfs/fonts

cd FontGenerator
if LD_LIBRARY_PATH=. python rasterizer_runner.py 2> /dev/null >/dev/null
then
	echo -e "\e[1;32mFonts generated\e[21;39m!"
else
	echo -e "\e[1;91mFont creator failed. Check log file.\e[21;39m"
fi
cd ..

cd romfs/fonts
gzip *.font 2> /dev/null
cd ../..

echo "ROMFS"
./create_romfs.sh

if test ! -e GUI-lib/libgui.a
then
	echo "Compiling GUI-lib..."
	cd GUI-lib
	./make_lib.sh
	cd ..
fi

echo "Compiling eCos application..."

OK=0
${GCC_PREFIX}gcc -Wno-psabi -g -I./ -g -I${KPATH}/install/include ${FILES} \
     -L${KPATH}/install/lib -Ttarget.ld ${CFLAGS} ${ADD_OPT} -nostdlib -o $OUTPUT_FILENAME && OK=1
if [ $OK -eq 0 ]
then
	echo -e "\e[1;91mCOMPILATION ERROR\e[21;39m!"
else
        ${GCC_PREFIX}objcopy -O binary $CONFIG $CONFIG.bin
	echo -e "\e[1;32mCOMPILATION SUCCESSFUL\e[21;39m!"
	sz=`ls -sh ./$CONFIG.bin | cut -f 1 -d ' '`
	echo -e "resulting binary size: \e[1;32m${sz}\e[21;39m"
fi

#count crc
BINCRC=`cksum $CONFIG.bin ${my_iso_file} | awk '{ print $1 }'`
LENGTH=$(stat -c '%s' $CONFIG.bin)

# get the major version numer
pushd src/ > /dev/null
    NEEO_VERSION=`grep -h "#define FW_MAJOR_VERSION " main.cpp | tr -s ' ' | cut -d\  -f 3- | tr -d \"`
popd > /dev/null
echo "Neeo major version: "$NEEO_VERSION
printf -v v "%08x" $NEEO_VERSION

#Composing binary file
BINCRC=`printf "%.8x " ${BINCRC}`
echo "0: "${BINCRC:6:2}${BINCRC:4:2}${BINCRC:2:2}${BINCRC:0:2} | xxd -r -g2 > $CONFIG.temp

LENGTH=`printf "%.8x " ${LENGTH}`
echo "0: "${LENGTH:6:2}${LENGTH:4:2}${LENGTH:2:2}${LENGTH:0:2} | xxd -r -g2 >> $CONFIG.temp

NEEO_VERSION=`printf "%.8x " ${NEEO_VERSION}`
echo "0: "${NEEO_VERSION:6:2}${NEEO_VERSION:4:2}${NEEO_VERSION:2:2}${NEEO_VERSION:0:2} | xxd -r -g2 >> $CONFIG.temp

cat $CONFIG.temp $CONFIG.bin > $CONFIG.temp2
mv $CONFIG.temp2 $CONFIG.bin
rm -rf $CONFIG.temp $CONFIG.temp2

fi # if [ ! -z "$FILES" ]
