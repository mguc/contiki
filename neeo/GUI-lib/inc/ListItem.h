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
 * Created on: 2014-12-05
 *     Author: dkochmanski@antmicro.com
 *             wstrozynski@antmicro.com
 *
 */

#ifndef GUILISTITEM_H_
#define GUILISTITEM_H_

#include "AbstractButton.h"
#include "GUIFont.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class ListItem : public AbstractButton {
public:
    enum ItemType
    {
        StdListField,
        LeftArrowField,
        RightArrowField,
        Dots,
        EmptyField
    } Type;

public:
    ListItem()
        : AbstractButton(), Type(StdListField), Description(""), DescriptionFont(0),
          DescriptionColor(COLOR_ARGB8888_LIGHTGRAY), ActiveDescriptionColor(COLOR_ARGB8888_BLACK) {};
    ListItem(int32_t x, int32_t y, int32_t width, int32_t height)
        : AbstractButton(x, y, width, height), Type(StdListField), Description(""), DescriptionFont(0),
          DescriptionColor(COLOR_ARGB8888_LIGHTGRAY), ActiveDescriptionColor(COLOR_ARGB8888_BLACK) {};
    ListItem(int32_t x, int32_t y, int32_t width, int32_t height, ItemType type)
        : AbstractButton(x, y, width, height), Type(type), Description(""), DescriptionFont(0),
          DescriptionColor(COLOR_ARGB8888_LIGHTGRAY), ActiveDescriptionColor(COLOR_ARGB8888_BLACK) {};
    ListItem(const ListItem& Obj)
        : AbstractButton(Obj), Type(Obj.Type), Description(""), DescriptionFont(0),
          DescriptionColor(COLOR_ARGB8888_LIGHTGRAY), ActiveDescriptionColor(COLOR_ARGB8888_BLACK) {};

    virtual ~ListItem();

    void SetDescription(const char* desc);
    void SetDescriptionFont(GUIFont const * font);
    void SetType(ItemType type);

    void SetDescriptionColor(uint32_t color);
    void SetActiveDescriptionColor(uint32_t color);

    uint32_t GetDescriptionColor();
    uint32_t GetActiveDescriptionColor();

    static ListItem* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void PrepareContent(ContentManager* contentManager);
    virtual void CancelPreparingContent(ContentManager* contentManager);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

protected:
    string Description;
    GUIFont const * DescriptionFont;
    uint32_t DescriptionColor, ActiveDescriptionColor;

    static ItemType ParseListItemTypeOrDefault(const char* value, ItemType defaultValue);
};

} /* namespace guilib */

#endif/* GUILISTITEM_H_ */
