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

#ifndef AMGSCROLLBAR_H_
#define AMGSCROLLBAR_H_

#include "Component.h"
#include "Image.h"
#include "Painter.h"
#include "tinyxml2.h"
#include "XMLSupport.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class ScrollBar: public Component {
public:
    ScrollBar()
    : Component(), BarColor(COLOR_ARGB8888_LIGHTGRAY), ScrollColor(COLOR_ARGB8888_LIGHTBLUE),
    ActiveScrollColor(COLOR_ARGB8888_BLUE), FrameColor(COLOR_ARGB8888_BLACK), SelectedFrameColor(COLOR_ARGB8888_BLACK),
    ScrollLength(0), MinValue(0), MaxValue(0), Position(0.0), Value(0), ReportedValue(0), ScrollImage(),
    onValueChange(), timeStamp(0) {
    }
    ;
    ScrollBar(int32_t x, int32_t y, int32_t width, int32_t height)
    : Component(x, y, width, height), BarColor(COLOR_ARGB8888_LIGHTGRAY), ScrollColor(COLOR_ARGB8888_LIGHTBLUE),
    ActiveScrollColor(COLOR_ARGB8888_BLUE), FrameColor(COLOR_ARGB8888_BLACK), SelectedFrameColor(COLOR_ARGB8888_BLACK),
    ScrollLength(0), MinValue(0), MaxValue(0), Position(0.0), Value(0), ReportedValue(0), ScrollImage(),
    onValueChange(), timeStamp(0) {
    }
    ;
    ScrollBar(int32_t x, int32_t y, int32_t width, int32_t height, int32_t min, int32_t max)
    : Component(x, y, width, height), BarColor(COLOR_ARGB8888_LIGHTGRAY), ScrollColor(COLOR_ARGB8888_LIGHTBLUE),
    ActiveScrollColor(COLOR_ARGB8888_BLUE), FrameColor(COLOR_ARGB8888_BLACK), SelectedFrameColor(COLOR_ARGB8888_BLACK),
    ScrollLength(0), MinValue(min), MaxValue(max), Position(0.0), Value(0), ReportedValue(0), ScrollImage(),
    onValueChange(), timeStamp(0) {
    }
    ;
    ScrollBar(const ScrollBar& Obj)
    : Component(Obj), BarColor(Obj.BarColor), ScrollColor(Obj.ScrollColor), ActiveScrollColor(Obj.ActiveScrollColor),
    FrameColor(Obj.FrameColor), SelectedFrameColor(Obj.SelectedFrameColor), ScrollLength(Obj.ScrollLength),
    MinValue(Obj.MinValue), MaxValue(Obj.MaxValue), Position(Obj.Position), Value(Obj.Value),
    ReportedValue(Obj.ReportedValue), ScrollImage(Obj.ScrollImage), onValueChange(Obj.onValueChange),
    timeStamp(Obj.timeStamp) {
    }

    virtual ~ScrollBar();

    ScrollBar& operator=(const ScrollBar& Obj);

    void SetScrollImage(const Image& image);
    Image* GetScrollImagePtr();

    void SetBarColorColor(uint32_t color);
    void SetScrollColor(uint32_t color);
    void SetActiveScrollColor(uint32_t color);
    void SetFrameColor(uint32_t color);
    void SetSelectedFrameColor(uint32_t color);
    void SetMinValue(float value);
    void SetMaxValue(float value);
    void SetValue(double value);

    uint32_t GetBarColorColor();
    uint32_t GetScrollColor();
    uint32_t GetActiveScrollColor();
    uint32_t GetFrameColor();
    uint32_t GetSelectedFrameColor();
    int32_t GetMinValue();
    int32_t GetMaxValue();
    float GetValue();


    virtual void OnPress();
    virtual void OnRelease();
    virtual void OnClick();

    void SetOnValueChangeEvent(const Event& event);

    virtual Touch::TouchResponse ProcessMove(int32_t StartX, int32_t StartY, int32_t DeltaX, int32_t DeltaY);
    virtual bool IsTouchPointInObject(int32_t x, int32_t y, int32_t modificator);

    static ScrollBar* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

protected:
    uint32_t BarColor, ScrollColor, ActiveScrollColor, FrameColor, SelectedFrameColor, ScrollLength;
    float MinValue, MaxValue;
    float Position, Value, ReportedValue;
    Image ScrollImage;
    Event onValueChange;
    uint64_t timeStamp;

    float ValueToPosition(float value);
    float PositionToValue(float position);
};

} /* namespace guilib */

#endif /* AMGSCROLLBAR_H_ */
