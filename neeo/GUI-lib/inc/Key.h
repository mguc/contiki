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
 * Created on: 23 Jul 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUI_LIB_SRC_KEY_H_
#define GUI_LIB_SRC_KEY_H_

#include <stdint.h>
#include "stl.h"
#include "Event.h"
#include "Component.h"
#include "XMLSupport.h"

namespace guilib {

class Key {
public:
    enum KeyState {
        NA,
        KeyReleased,
        KeyPressed
    };

    virtual ~Key();
    Key(): ID(""), onPress(){};

    static Key BuildFromXml(Manager* man, XMLElement* xmlElement);

    void SetID(const char* id);
    void SetOnPressEvent(const Event& event);
    void SetOnReleaseEvent(const Event& event);
    void SetOnLongPressEvent(const Event& event);
    void SetOnLongPressRepeatEvent(const Event& event);

    void SetEventSenderPtr(void* sender);

    KeyState TriggerOnPressEvent();
    KeyState TriggerOnReleaseEvent();
    KeyState TriggerOnLongPressEvent();
    KeyState TriggerOnLongPressRepeatEvent();

    const char* GetID();

private:
    string ID;
    Event onPress, onRelease, onLongPress, onLongPressRepeat;
};

} /* namespace guilib */

#endif /* GUI_LIB_SRC_KEY_H_ */
