CONTIKI_PROJECT = tr2-com-firmware version

all: $(CONTIKI_PROJECT)

version: ../.git/HEAD ../.git/index
	@echo "const char *version = \"$(shell git log | head -1 | cut -f 2 -d ' ' | cut -b 1-8)\";" > version.h

CONTIKI = ..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
