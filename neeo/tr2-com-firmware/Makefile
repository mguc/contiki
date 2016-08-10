CONTIKI_PROJECT = git_revision tr2-com-firmware major_version
FIRMWARE_VERSION := `grep -h "\#define FW_MAJOR_VERSION " tr2-com-firmware.c | tr -s ' ' | cut -d\  -f 3- | tr -d \"`
all: $(CONTIKI_PROJECT)
APPS += er-coap
APPS += rest-engine

-include Makefile.user

ifeq ($(TARGET),)
  -include Makefile.target
endif

.PHONY: git_revision
git_revision:
	@echo "const char *version = \"$(shell git rev-parse HEAD | cut -b 1-8)\";" > version.h

.PHONY: major_version
major_version:
	@printf "0: %.8x" $(FIRMWARE_VERSION) | sed -E 's/0: (..)(..)(..)(..)/0: \4\3\2\1/' | xxd -r -g0 >> .tmp.version
	@dd if=.tmp.version conv=notrunc of=tr2-com-firmware.$(TARGET).bin bs=1 count=4 status=none
	@rm .tmp.version

deploy: $(CONTIKI_PROJECT)
	cp tr2-com-firmware.$(TARGET).bin $(DESTINATION_FILE)


CONTIKI = ../..
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\" -fsigned-char

PROJECT_SOURCEFILES += serial-protocol.c
PROJECT_SOURCEFILES += rtc.c
PROJECT_SOURCEFILES += rest_server.c
PROJECT_SOURCEFILES += tr2-debug.c
PROJECT_SOURCEFILES += rimestats.c
PROJECT_SOURCEFILES += io_power_control.c

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
