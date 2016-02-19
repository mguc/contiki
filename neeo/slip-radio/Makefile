CONTIKI_PROJECT=slip-radio
FIRMWARE_VERSION := `grep -h "\#define FW_MAJOR_VERSION " slip-radio.c | tr -s ' ' | cut -d\  -f 3- | tr -d \"`
all: $(CONTIKI_PROJECT)
APPS = slip-cmd

-include Makefile.user

ifeq ($(TARGET),)
  -include Makefile.target
endif

ifeq ($(WITH_FEM),1)
  CFLAGS += -DRADIO_TEST_MODE=RADIO_TEST_MODE_HIGH_PWR
endif

.PHONY: git_revision
git_revision:
	@echo "const char *version = \"$(shell git rev-parse HEAD | cut -b 1-8)\";" > version.h

.PHONY: major_version
major_version:
	@printf "0: %.8x" $(FIRMWARE_VERSION) | sed -E 's/0: (..)(..)(..)(..)/0: \4\3\2\1/' | xxd -r -g0 >> .tmp.version
	@dd if=.tmp.version conv=notrunc of=tr2-com-firmware.jn5168.bin bs=1 count=4 status=none
	@rm .tmp.version

CONTIKI=../..
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

PROJECT_SOURCEFILES += slip-net.c
PROJECT_SOURCEFILES += no-framer.c
PROJECT_SOURCEFILES += led_control.c
PROJECT_SOURCEFILES += ir_control.c
PROJECT_SOURCEFILES += ir-learn.c
PROJECT_SOURCEFILES += rimestats.c

CONTIKI_WITH_RPL = 0
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
