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
 * Created on: 2014-09-04
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUIABSTRACTBUTTON_H_
#define GUIABSTRACTBUTTON_H_

#include "Component.h"
#include "GUIFont.h"
#include "stl.h"
#include "GUILib.h"
#include "Image.h"
#include "Painter.h"

namespace guilib {

class AbstractButton : public Component {
public:
    AbstractButton()
        : Component(), Text(""), ButtonImage(), ButtonFont(0), timeStamp(0), longTouchActive(false), onLongPress(), onLongPressRepeat(){
    };
    AbstractButton(int32_t x, int32_t y, int32_t width, int32_t height)
        : Component(x,y,width, height), Text(""), ButtonImage(), ButtonFont(0), timeStamp(0), longTouchActive(false), onLongPress(), onLongPressRepeat(){
    };
    AbstractButton(const AbstractButton& Obj)
        : Component(Obj), Text(Obj.Text), ButtonImage(Obj.ButtonImage), ButtonFont(Obj.ButtonFont), timeStamp(0), longTouchActive(false), onLongPress(), onLongPressRepeat(){
    };

    virtual ~AbstractButton();

    AbstractButton& operator=(const AbstractButton& Obj);

    void SetText(const char * text);
    void SetImage(const Image& image);
    void SetTextFont(GUIFont const * font);
    void SetOnLongPressEvent(const Event& event);
    void SetOnLongPressRepeatEvent(const Event& event);

    void ClrButtonFont();

    virtual void OnPress();
    virtual void OnRelease();
    virtual void OnClick();

    virtual void PrepareContent(ContentManager* contentManager) {};
    virtual void CancelPreparingContent(ContentManager* contentManager) {};

    Touch::TouchResponse ProcessMove(int32_t StartX, int32_t StartY, int32_t DeltaX, int32_t DeltaY);

    GUIFont const * GetButtonFont();
    Image* GetImagePtr();

protected:
    string Text;
    Image ButtonImage;
    GUIFont const * ButtonFont;
    uint64_t timeStamp;

    //Long touch support
    bool longTouchActive;
    Event onLongPress, onLongPressRepeat;
};

} /* namespace guilib */

#endif /* GUIABSTRACTBUTTON_H_ */
