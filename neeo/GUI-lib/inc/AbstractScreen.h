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
 * Created on: 2014-11-06
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUIABSTRACTSCREEN_H_
#define GUIABSTRACTSCREEN_H_

#include <math.h>

#include "Component.h"
#include "Image.h"
#include "stl.h"
#include "Painter.h"
#include "Panel.h"
#include "Key.h"

namespace guilib {

class AbstractScreen : public Component {
public:
    AbstractScreen()
        : Component(), BackgroundImage(), activeChild(0), childDropped(false),
          globalPanelVisible(false), header(0), onSlideToLeft(), onSlideToRight(), collectionSize(0), positionInCollection(0){
          BackgroundColor = COLOR_ARGB8888_WHITE;
    };
    AbstractScreen(const AbstractScreen& Obj)
        : Component(Obj), BackgroundImage(Obj.BackgroundImage), activeChild(Obj.activeChild),
          childDropped(Obj.childDropped), globalPanelVisible(Obj.activeChild), header(0), onSlideToLeft(),
          onSlideToRight(), collectionSize(0), positionInCollection(0){
    }
    AbstractScreen(int32_t x, int32_t y, int32_t width, int32_t height)
        : Component(x, y, width, height), BackgroundImage(), activeChild(0),
          childDropped(false), globalPanelVisible(false), header(0), onSlideToLeft(), onSlideToRight(), collectionSize(0), positionInCollection(0){
          BackgroundColor = COLOR_ARGB8888_WHITE;
    };

    virtual ~AbstractScreen();

    AbstractScreen& operator=(const AbstractScreen& Obj);

    void SetBackgroundImage(const Image& image);
    void SetGlobalPanelVisibility(bool visible);
    bool GetGlobalPanelVisibility();
    virtual Component* GetElement(uint32_t index) = 0;
    virtual Component* GetElement(const char* id) = 0;
    virtual Component* GetLastElement() = 0;

    uint32_t GetCollectionSize();
    uint32_t GetPositionInCollection();

    void SetCollectionSize(uint32_t size);
    void SetPositionInCollection(uint32_t position);

    void SetOnSlideToLeftEvent(const Event& event);
    void SetOnSlideToRightEvent(const Event& event);

    void SetHeader(Panel* headerPtr);
    Panel* GetHeader();

    virtual void ScreenInit() = 0;
    virtual void CheckPlacement() = 0;

    Key::KeyState PressKey(const char* id);
    Key::KeyState ReleaseKey(const char* id);
    Key::KeyState LongPressKey(const char* id);
    Key::KeyState LongPressRepeatKey(const char* id);

    void AddKeyToCollection(Key key);

protected:
    Image BackgroundImage;
    Component* activeChild;
    bool childDropped, globalPanelVisible;
    Panel* header;
    Event onSlideToLeft, onSlideToRight;
    uint32_t collectionSize, positionInCollection;
    vector<Key> KeyCollection;
};

} /* namespace guilib */

#endif /* GUIABSTRACTSCREEN_H_ */
