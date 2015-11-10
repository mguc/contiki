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
 * Created on: 11 Aug 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef SRC_CONTAINER_H_
#define SRC_CONTAINER_H_

#include "Component.h"
#include "Image.h"
#include "stl.h"
#include "Painter.h"
#include "tinyxml2.h"

namespace guilib {
class Manager;

class Container : public Component {
public:
    Container()
        : Component(), Visible(true), childDropped(false), BackgroundImage(NULL), activeChild(0) {};
    Container(int32_t x, int32_t y, int32_t width, int32_t height)
        : Component(x, y, width, height), Visible(true), childDropped(false),
          BackgroundImage(NULL), activeChild(0) {};
    Container(const Container& Obj)
        : Component(Obj), Visible(true), childDropped(false),
          BackgroundImage(NULL), activeChild(0) {};
    virtual ~Container();

    virtual void SetLayer(uint32_t layer);

    virtual void CheckPlacement();
    vector<Component*>& GetElements();
    Component* GetElement(const char* id);
    virtual void AddElement(Component* el);
    void SetBackgroundImage(Image *image);

    void Show();
    void Hide();
    void SetVisible(bool visible);
    bool IsVisible();

    virtual Touch::TouchResponse ProcessTouch(const Touch& tp, int32_t ParentX, int32_t ParentY, int32_t modificator = 0);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight) = 0;

protected:
    vector<Component*> Elements;
    bool Visible, childDropped;
    Image *BackgroundImage;
    Component* activeChild;
};

} /* namespace guilib */

#endif /* SRC_CONTAINER_H_ */
