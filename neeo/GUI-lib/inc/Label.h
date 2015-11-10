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
 * Created on: 2014-09-22
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUITEXT_H_
#define GUITEXT_H_

#include "Component.h"
#include "GUIFont.h"
#include "stl.h"
#include "Painter.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class Label : public Component{
public:
    enum TextHorizontalAlignment{
        Left,
        Right,
        Center
    };

protected:
    string Text;
    uint32_t FrameColor;
    TextHorizontalAlignment HorizontalAlignment;
    GUIFont const * TextFont;

public:
    Label()
        : Component(),Text(""), FrameColor(COLOR_ARGB8888_TRANSPARENT),
          HorizontalAlignment(Left), TextFont(0){
    };
    Label(int32_t x, int32_t y, int32_t width, int32_t height)
        : Component(x, y, width, height),Text(""), FrameColor(COLOR_ARGB8888_TRANSPARENT),
          HorizontalAlignment(Left), TextFont(0){
    };
    Label(int32_t x, int32_t y, int32_t width, int32_t height, const char * text,TextHorizontalAlignment alignment)
        : Component(x, y, width, height), Text(text), FrameColor(COLOR_ARGB8888_TRANSPARENT),
          HorizontalAlignment(alignment), TextFont(0){
    };
    Label(const Label& Obj)
        : Component(Obj), Text(Obj.Text), FrameColor(Obj.FrameColor),
          HorizontalAlignment(Obj.HorizontalAlignment),
          TextFont(Obj.TextFont){
    }

    virtual ~Label();

    Label& operator=(const Label& Obj);

    void SetText(const char * text);
    void SetHorizontalAlignment(TextHorizontalAlignment alignment);
    void SetFrameColor(uint32_t color);
    void SetTextFont(GUIFont const * font);

    string& GetText();

    TextHorizontalAlignment GetMode();
    uint32_t GetFrameColor();
    GUIFont const * GetTextFont();

    static Label* BuildFromXml(Manager* man, XMLElement* xmlElement);
    virtual bool IsTouchPointInObject(int32_t x, int32_t y, int32_t modificator = 0);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);
};

} /* namespace guilib */

#endif /* GUITEXTVIEW_H_ */
