CONTIKI_PROJECT = udp-broadcast-sender udp-broadcast-receiver
all: $(CONTIKI_PROJECT)

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

ifeq ($(WITH_FEM),1)
  CFLAGS += -DRADIO_TEST_MODE=RADIO_TEST_MODE_HIGH_PWR
endif

APPS+=replong

CONTIKI=../..

SMALL=1
#IAR=1

PROJECT_SOURCEFILES += rimestats.c

include $(CONTIKI)/Makefile.include
