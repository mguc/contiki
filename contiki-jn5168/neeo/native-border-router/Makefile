CONTIKI_PROJECT=border-router

-include Makefile.user

ifeq ($(TARGET),)
  -include Makefile.target
endif

all: $(CONTIKI_PROJECT)
cross: CROSS_COMPILE := $(CROSS_COMPILER)
cross: $(CONTIKI_PROJECT)

.PHONY: deploy
deploy:
	@scp $(CONTIKI_PROJECT).$(TARGET) root@$(TARGET_IP):$(TARGET_PATH)

APPS = slip-cmd
APPS += rest-http

CONTIKI=../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\" -fsigned-char

PROJECT_SOURCEFILES += border-router-cmds.c
PROJECT_SOURCEFILES += tun-bridge.c
PROJECT_SOURCEFILES += border-router-rdc.c
PROJECT_SOURCEFILES += slip-config.c
PROJECT_SOURCEFILES += slip-dev.c
PROJECT_SOURCEFILES += rest_resources.c
PROJECT_SOURCEFILES += log_helper.c

CONTIKI_WITH_IPV6 = 1

include $(CONTIKI)/Makefile.include
