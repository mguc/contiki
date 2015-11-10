/********************************************************************
 * 
 * CONFIDENTIAL
 * 
 * ---
 * 
 * (c) 2014-2015 Antmicro Ltd <antmicro.com>
 * All Rights Reserved.
 * 
 * NOTICE: All information contained herein is, and remains
 * the property of Antmicro Ltd.
 * The intellectual and technical concepts contained herein
 * are proprietary to Antmicro Ltd and are protected by trade secret
 * or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Antmicro Ltd.
 *
 * ---
 *
 * Created on: 9 Jun 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUI_LIB_INC_XMLSUPPORT_H_
#define GUI_LIB_INC_XMLSUPPORT_H_

#include "Label.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {

class XMLSupport {
public:
    static uint32_t GetAttributeOrDefault(XMLElement* element, const char* attributeName, uint32_t defaultValue);
    static bool GetAttributeOrDefault(XMLElement* element, const char* attributeName, bool defaultValue);
    static const char* GetAttributeOrDefault(XMLElement* element, const char* attributeName, const char* defaultValue);
    static enum Label::TextHorizontalAlignment ParseAlignmentOrDefault(const char* value, enum Label::TextHorizontalAlignment defaultValue);
    static bool TryGetAttribute(XMLElement* element, const char* attributeName, const char** value);
    static bool TryGetIntAttribute(XMLElement* element, const char* attributeName, int32_t* value);
};

} /* namespace stmgui */

#endif /* GUI_LIB_INC_XMLSUPPORT_H_ */
