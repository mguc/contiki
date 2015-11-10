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
 * Created on: 2014-09-18
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUISWITCHBUTTON_H_
#define GUISWITCHBUTTON_H_

#include "AbstractButton.h"
#include "stl.h"
#include "Event.h"
#include "Painter.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class SwitchButton : public AbstractButton{
public:
    SwitchButton()
        : AbstractButton(), SwitchColor(COLOR_ARGB8888_GRAY),
          ActiveSwitchColor(COLOR_ARGB8888_LIGHTBLUE), TextColor(COLOR_ARGB8888_BLACK),
          ActiveTextColor(COLOR_ARGB8888_BLACK), FrameColor(COLOR_ARGB8888_BLACK), SelectedFrameColor(COLOR_ARGB8888_BLACK),
          switchState(false), previousSwitchState(false), onSwitchON(), onSwitchOFF(), TextWidth(0) {
        BackgroundColor=COLOR_ARGB8888_LIGHTGRAY;
    };
    SwitchButton(int32_t x, int32_t y, int32_t width, int32_t height)
        : AbstractButton(x, y, width, height), SwitchColor(COLOR_ARGB8888_GRAY),
          ActiveSwitchColor(COLOR_ARGB8888_LIGHTBLUE),    TextColor(COLOR_ARGB8888_BLACK),
          ActiveTextColor(COLOR_ARGB8888_BLACK), FrameColor(COLOR_ARGB8888_BLACK),
          SelectedFrameColor(COLOR_ARGB8888_BLACK), switchState(false), previousSwitchState(false), onSwitchON(), onSwitchOFF(), TextWidth(0) {
        BackgroundColor=COLOR_ARGB8888_LIGHTGRAY;
    };

    virtual ~SwitchButton();

    virtual void OnPress();
    virtual void OnRelease();
    virtual void OnClick();

    void SetSwitchColor(uint32_t color);
    void SetActiveSwitchColor(uint32_t color);
    void SetTextColor(uint32_t color);
    void SetActiveTextColor(uint32_t color);
    void SetFrameColor(uint32_t color);
    void SetSelectedFrameColor(uint32_t color);
    void SetSwitchState(bool state);

    uint32_t GetSwitchColor();
    uint32_t GetActiveSwitchColor();
    uint32_t GetTextColor();
    uint32_t GetActiveTextColor();
    uint32_t GetFrameColor();
    uint32_t GetSelectedFrameColor();
    bool GetSwitchState();

    void setOnSwitchONEvent(const Event& event);
    void setOnSwitchOFFEvent(const Event& event);

    static SwitchButton* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

protected:
    uint32_t SwitchColor, ActiveSwitchColor, TextColor, ActiveTextColor, FrameColor, SelectedFrameColor;
    bool switchState, previousSwitchState;
    Event onSwitchON, onSwitchOFF;
    int32_t TextWidth;

    virtual Touch::TouchResponse ProcessMove( int32_t StartX, int32_t StartY, int32_t DeltaX, int32_t DeltaY);
};

} /* namespace guilib */

#endif /* GUISWITCHBUTTON_H_ */
