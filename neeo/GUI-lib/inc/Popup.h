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
 * Created on: 2015-01-23
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef ANTMICRO_GUI_GUI_LIB_GUIPOPUP_H_
#define ANTMICRO_GUI_GUI_LIB_GUIPOPUP_H_

#include "Container.h"
#include "stl.h"
#include "Label.h"
#include "Painter.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class Popup : public Container {
public:
    Popup()
    : Container(), Message(NULL), timeStamp(0){
    }
    Popup (int32_t x, int32_t y, int32_t width, int32_t height)
    : Container(x, y, width, height), Message(NULL), timeStamp(0) {
    }

    //TODO add copy constructor and operator '='

    virtual ~Popup();

    void SetMessage(const char* message);
    void SetMessagePtr(Label* message);
    void SetTimestamp(uint64_t currentTimestamp);

    Label* GetMessagePtr();
    uint64_t GetTimetamp();

    void SetLayer(uint32_t layer);

    static Popup* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

protected:
    Label* Message;
    uint64_t timeStamp;
};

} /* namespace guilib */

#endif /* ANTMICRO_GUI_GUI_LIB_GUIPOPUP_H_ */
