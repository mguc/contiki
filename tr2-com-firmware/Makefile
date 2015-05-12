CONTIKI_PROJECT = version tr2-com-firmware

all: $(CONTIKI_PROJECT)

version:
	@echo "const char *version = \"$(shell git rev-parse HEAD | cut -b 1-8)\";" > version.h

PROJECT_SOURCEFILES += serial-protocol.c
CONTIKI = ..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
