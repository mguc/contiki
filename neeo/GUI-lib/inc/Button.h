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

#ifndef GUIBUTTON_H_
#define GUIBUTTON_H_

#include "AbstractButton.h"
#include "stl.h"
#include "GUIFont.h"
#include "Painter.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class Button : public AbstractButton  {
public:
    Button()
    : AbstractButton(), IcoColor(COLOR_ARGB8888_BLACK), ActiveIcoColor(COLOR_ARGB8888_BLACK),
      FrameColor(COLOR_ARGB8888_BLACK), SelectedFrameColor(COLOR_ARGB8888_BLACK), IcoChar(-1), TextTopOffset(0),
      IcoFont(0), imageCentered(false) {
        BackgroundColor=COLOR_ARGB8888_LIGHTGRAY;
    };
    Button(int32_t x, int32_t y, int32_t width, int32_t height)
    : AbstractButton(x, y, width, height), IcoColor(COLOR_ARGB8888_BLACK), ActiveIcoColor(COLOR_ARGB8888_BLACK),
      FrameColor(COLOR_ARGB8888_BLACK), SelectedFrameColor(COLOR_ARGB8888_BLACK), IcoChar(-1), TextTopOffset(0),
      IcoFont(0), imageCentered(false) {
        BackgroundColor=COLOR_ARGB8888_LIGHTGRAY;
    };

    virtual ~Button();

    void SetTextColor(uint32_t color);
    void SetIcoColor(uint32_t color);
    void SetActiveTextColor(uint32_t color);
    void SetActiveIcoColor(uint32_t color);
    void SetFrameColor(uint32_t color);
    void SetSelectedFrameColor(uint32_t color);
    void SetIcoFont(GUIFont const * font);
    void SetIcoChar(int16_t textIco);
    void SetImagePos(int32_t x, int32_t y);
    void SetTextTopOffset(int32_t value);
    void SetImageCentered(bool isCentered);
    virtual void SetSize(int32_t width, int32_t height);

    void ClrIcoFont();
    void ClrIcoChar();

    uint32_t GetTextColor();
    uint32_t GetIcoColor();
    uint32_t GetActiveTextColor();
    uint32_t GetActiveIcoColor();
    uint32_t GetFrameColor();
    uint32_t GetSelectedFrameColor();
    GUIFont const * GetIcoFont();

    static Button* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

protected:
    uint32_t IcoColor, ActiveIcoColor, FrameColor, SelectedFrameColor;
    int16_t IcoChar;
    int32_t TextTopOffset;
    GUIFont const * IcoFont;
    bool imageCentered;
};

} /* namespace guilib */

#endif /* GUIBUTTON_H_ */
