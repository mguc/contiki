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
 * Created on: 29 Jul 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUI_LIB_INC_GRIDROW_H_
#define GUI_LIB_INC_GRIDROW_H_

#include "AbstractButton.h"
#include "stl.h"
#include "Painter.h"
#include "tinyxml2.h"

namespace guilib {

class GridRow : public AbstractButton {
public:
    GridRow()
    : AbstractButton(), ElementWidth(0), childDropped(false), ignoreTouchModificator(false), activeChild(0) {

    }
    GridRow(int32_t x, int32_t y, int32_t width, int32_t height)
    : AbstractButton(x, y, width, height), ElementWidth(0), childDropped(false), ignoreTouchModificator(false), activeChild(0) {

    }
    virtual ~GridRow();

    virtual void AddElement(AbstractButton *item);

    virtual void SetSize(int32_t width, int32_t height);

    virtual Component* GetElement(const char* id);
    virtual Touch::TouchResponse ProcessTouch(const Touch& tp, int32_t ParentX, int32_t ParentY, int32_t modificator = 0);

    virtual void CheckPlacement();
    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

    static GridRow* BuildFromXml(Manager* man, XMLElement* xmlElement);

    void SetElementWidth(int32_t elementWidth);

protected:
    vector<Component*> Elements;
    int32_t ElementWidth;
    bool childDropped, ignoreTouchModificator;
    Component* activeChild;

    void reorderElements();
};

} /* namespace guilib */

#endif /* GUI_LIB_INC_GRIDROW_H_ */
