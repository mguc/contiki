/*
 * FontManager.h
 *
 *  Created on: 7 Mar 2015
 *      Author: ws
 */

#include "../GUI-lib/inc/GUIFont.h"
#include "tinyxml2.h"
#include "file_download.h"

using namespace tinyxml2;
using namespace guilib;

int32_t GetFontsFromXML(const char* XMLDoc, size_t len, bool tryDownload);
GUIFont* BuildFont(const string& FontName,  bool tryDownload);
