/*
 * FontManager.c
 *
 *  Created on: 7 Mar 2015
 *      Author: ws
 */

#include "../include/FontManager.h"

#include <cyg/libc/string/string.h>
#include "../GUI-lib/inc/Manager.h"
#include "stl.h"
#include "log_helper.h"

extern Manager* DispMan;
using namespace guilib;

int32_t GetFontsFromXML(const char* XMLDoc, size_t len, bool tryDownload){
    XMLDocument doc;

    XMLError Error = doc.Parse(XMLDoc, len);

    if (Error == XML_SUCCESS) {
        XMLNode * Root = doc.LastChild();

        XMLElement* fontElement = 0;
        XMLElement* fontElements = Root->FirstChildElement("font-styles");
        if(fontElements) fontElement = fontElements->FirstChildElement("font-style");
        while(fontElement != 0){
            string FontName = fontElement->Attribute("name");
            GUIFont* font = BuildFont(FontName, tryDownload);
            if(font) {
                log_msg(LOG_INFO, "FontManager", "Adding FONT: %s", FontName.c_str());
                DispMan->AddFontToFontContainer(FontName, font);
            }
            fontElement = (XMLElement*) fontElement->NextSiblingElement("font-style");
        }
    }
    doc.Clear();
    return 0;
}

inline bool exists (const string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

GUIFont* BuildFont(const string& FontName, bool tryDownload){
    //uint32_t value = 0;
    //XMLError ErrorCode;

    string path;
    //Check if font is present inside romfs
    path.append("/rom/fonts/");
    path.append(FontName);
    path.append(".font");

    GUIFont* TFont = 0;
    if (!exists(path)) path.append(".gz");
    if(exists(path)) {

        TFont = new GUIFont(path.c_str());

    } else if (tryDownload) { //try download

        path.clear();
        path.append("fonts/");
        path.append(FontName);
        path.append(".font");

        uint8_t* Data;
        uint32_t Size;

        log_msg(LOG_INFO, "FontManager", "Downloading %s", path.c_str());

        int retVal = wifi_download(path.c_str(), "127.0.0.1", &Size, &Data);
        if(retVal < 0) {printf("Couldn't download! \r\n"); return 0;}
        if(Data != 0) TFont = new GUIFont((uint16_t*)Data);
        else TFont = 0;

    } else {
        TFont = 0;
    }

    if(TFont == 0) log_msg(LOG_ERROR, "FontManager", "%s doesn't exist", path.c_str());

    return TFont;
}
