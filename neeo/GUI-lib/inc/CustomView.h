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
 * Created on: 2014-09-11
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUICUSTOMVIEW_H_
#define GUICUSTOMVIEW_H_

#include <math.h>

#include "AbstractScreen.h"
#include "stl.h"
#include "Painter.h"

namespace guilib {

class CustomView : public AbstractScreen {
public:
    CustomView()
    : AbstractScreen() {
    };
    CustomView(int32_t x, int32_t y, int32_t width, int32_t height)
    : AbstractScreen(x, y, width, height) {
    };
    virtual ~CustomView();

    void AddElement(Component* element);
    virtual Component* GetElement(uint32_t index);
    virtual Component* GetElement(const char* id);
    virtual Component* GetLastElement();

    //Touch
    virtual Touch::TouchResponse ProcessTouch(const Touch& tp, int32_t ParentX, int32_t ParentY, int32_t modificator = 0);

    virtual void CheckPlacement();

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);
    virtual void ScreenInit() {}

protected:
    vector<Component*> Elements;

};

} /* namespace guilib */

#endif /* GUICUSTOMVIEW_H_ */
