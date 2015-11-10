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
 * Created on: 2014-09-01
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUICOMPONENT_H_
#define GUICOMPONENT_H_

#include "stl.h"
#include <stdint.h>
#include "Definitions.h"
#include "Event.h"
#include "Painter.h"
#include "Touch.h"

namespace guilib {

class Component {
public:
    enum ComponentState
    {
        Off=0,
        On,
        Pressed,
        Released,
        OnAndSelected,
        OffAndSelected
    };

    void* operator new(size_t size);
    void operator delete(void* ptr);

    Component()
        : ID(""), Layer(0), X(0), Y(0), Height(0), Width(0), ForegroundColor(COLOR_ARGB8888_GRAY),
          ActiveForegroundColor(COLOR_ARGB8888_GRAY), BackgroundColor(COLOR_ARGB8888_TRANSPARENT),
          ActiveBackgroundColor(COLOR_ARGB8888_TRANSPARENT), State(Off), onPress(), onRelease(), onClick(),
          touchActive(false), childDroppedTouch(false), redrawList(false) {
    };
    Component(int32_t x, int32_t y, int32_t width, int32_t height)
        : ID(""), Layer(0), X(x), Y(y), Height(height), Width(width), ForegroundColor(COLOR_ARGB8888_GRAY),
          ActiveForegroundColor(COLOR_ARGB8888_GRAY), BackgroundColor(COLOR_ARGB8888_TRANSPARENT),
          ActiveBackgroundColor(COLOR_ARGB8888_TRANSPARENT), State(Off), onPress(), onRelease(), onClick(),
          touchActive(false), childDroppedTouch(false), redrawList(false) {
    };
    virtual ~Component();

    virtual void SetLayer(uint8_t layer);
    void SetID(const char* id);
    virtual void SetPosition(int32_t x, int32_t y);
    virtual void SetSize(int32_t width, int32_t height);
    virtual void SetBackgroundColor(uint32_t color);
    virtual void SetForegroundColor(uint32_t color);
    virtual void SetActiveBackgroundColor(uint32_t color);
    virtual void SetActiveForegroundColor(uint32_t color);
    void SetState(ComponentState state);

    uint8_t GetLayer();
    const char* GetID();
    int32_t GetX();
    int32_t GetY();
    int32_t GetWidth();
    int32_t GetHeight();
    uint32_t GetBackgroundColor();
    uint32_t GetForegroundColor();
    uint32_t GetActiveBackgroundColor();
    uint32_t GetActiveForegroundColor();
    virtual uint32_t GetCurrentBackgroundColor();
    virtual uint32_t GetCurrentForegroundColor();
    ComponentState GetState();

    virtual void OnPress();
    virtual void OnRelease();
    virtual void OnClick();

    void SetOnPressEvent(const Event& event);
    void SetOnReleaseEvent(const Event& event);
    void SetOnClickEvent(const Event& event);

    virtual Touch::TouchResponse ProcessTouch(const Touch& tp, int32_t ParentX, int32_t ParentY, int32_t modificator = 0);
    virtual bool IsTouchPointInObject(int32_t x, int32_t y, int32_t modificator = 0);

    virtual void CheckPlacement();

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight) = 0;

    // Static aux functions
    static int32_t Clamp(int32_t val, int32_t left, int32_t right);
    static bool checkSizeValidity(Component* parent, Component* child);

protected:
    string ID;
    uint8_t Layer;
    int32_t X, Y, Height, Width;
    uint32_t ForegroundColor, ActiveForegroundColor, BackgroundColor, ActiveBackgroundColor;
    ComponentState State;
    Event onPress, onRelease, onClick;
    bool touchActive, childDroppedTouch, redrawList;

    virtual Touch::TouchResponse ProcessMove(int32_t StartX, int32_t StartY, int32_t DeltaX, int32_t DeltaY);
};

} /* namespace guilib */

#endif /* GUICOMPONENT_H_ */
