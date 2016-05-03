CONTIKI_PROJECT = channel_control client
all: $(CONTIKI_PROJECT)

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI=../../..

SMALL=1

PROJECT_SOURCEFILES += rimestats.c

include $(CONTIKI)/Makefile.include
