#include <string.h>
#include "app_config.h"
#include "log_helper.h"
#include "stl.h"
#include "common_types.h"
#include "tinyxml2.h"
#include "XMLSupport.h"

using namespace tinyxml2;

AppConfig appMainConfig;

AppConfig::AppConfig():wifiSsid(""),wifiPass(""),guiLocation(""),cp6Ipv4(""),cp6Ipv6(""),powerSaveTimeout(10)
{

}

AppConfig::AppConfig(file_t* configFile)
{
    this->load(configFile);
}

int AppConfig::load(file_t* configFile)
{
    XMLDocument doc( true, COLLAPSE_WHITESPACE );
    XMLError error = doc.Parse((const char*)configFile->data, configFile->size);

    if(error == XML_SUCCESS)
    {
        XMLElement * cfg = doc.FirstChildElement("doc")->FirstChildElement("appConfig");
        if(cfg != 0)
        {
            this->powerSaveTimeout = (int32_t)XMLSupport::GetAttributeOrDefault(cfg, "powerSaveTimout", (uint32_t)10);
            this->wifiSsid = (const char *)XMLSupport::GetAttributeOrDefault(cfg, "wifiSsid", "");
            this->wifiPass = (const char *)XMLSupport::GetAttributeOrDefault(cfg, "wifiPass", "");
            this->guiLocation = (const char *)XMLSupport::GetAttributeOrDefault(cfg, "guiLoc", "");
            this->cp6Ipv4 = (const char *)XMLSupport::GetAttributeOrDefault(cfg, "cp6Ipv4", "");
            this->cp6Ipv6 = (const char *)XMLSupport::GetAttributeOrDefault(cfg, "cp6Ipv6", "");
            log_msg(LOG_INFO, __cfunc__, "powerSaveTimeout: %u", this->powerSaveTimeout);
            doc.Clear();
            return 0;
        }
        else
        {
            log_msg(LOG_ERROR, __cfunc__, "Element 'appConfig' in gui.xml not found!");
        }
    }
    else
    {
        log_msg(LOG_ERROR, __cfunc__, "Error in parsing gui.xml for appConfig.");
    }
    doc.Clear();

    return -1;
}