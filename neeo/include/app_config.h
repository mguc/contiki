#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "stl.h"
#include "common_types.h"

class AppConfig
{
public:
    AppConfig();
    AppConfig(file_t* configFile);
    string wifiSsid;
    string wifiPass;
    string guiLocation;
    string cp6Ipv4;
    string cp6Ipv6;
    int32_t powerSaveTimeout;
    int load(file_t* configFile);
};

extern AppConfig appMainConfig;


#endif
