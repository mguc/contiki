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
 * Created on: 28 Jul 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUI_LIB_SRC_VERTICALSCROLLVIEW_H_
#define GUI_LIB_SRC_VERTICALSCROLLVIEW_H_

#include "AbstractScreen.h"
#include "ListItem.h"
#include "stl.h"
#include "Painter.h"

namespace guilib {

class VerticalScrollView : public AbstractScreen {
public:

    VerticalScrollView()
        : AbstractScreen(), Scroll(0), ScrollMax(0), ScrollChange(0), prevDeltaY(0), prevDeltaX(0), itemsHeight(0),
          animation(false), dSpeed(0), ElementColor(COLOR_ARGB8888_BROWN), SplitLineColor(COLOR_ARGB8888_TRANSPARENT),
          scrollingTimeStamp(0), scrollingEnabled(true), overscrollBarEnabled(false), overscrollBarSize(50),
          currentOverscrollBarSize(0), overscrollBarColor(COLOR_ARGB8888_LIGHTGRAY), currentSample(0),
          scrollIndicatorTimestamp(0), scrollIndicatorColor(0), scrollIndicatorOpacity(0) {
    };

    VerticalScrollView(int32_t x, int32_t y, int32_t width, int32_t height)
        : AbstractScreen(x, y, width, height), Scroll(0), ScrollMax(0), ScrollChange(0),
          prevDeltaY(0), prevDeltaX(0), itemsHeight(0), animation(false), dSpeed(0), ElementColor(COLOR_ARGB8888_BROWN),
          SplitLineColor(COLOR_ARGB8888_TRANSPARENT), scrollingTimeStamp(0), scrollingEnabled(true),
          overscrollBarEnabled(false), overscrollBarSize(50), currentOverscrollBarSize(0),
          overscrollBarColor(COLOR_ARGB8888_LIGHTGRAY), currentSample(0), scrollIndicatorTimestamp(0),
          scrollIndicatorColor(0), scrollIndicatorOpacity(0) {
    };

    virtual ~VerticalScrollView();

    virtual void AddElement(AbstractButton *item);

    void SetScrollingValue(int32_t scrollVal);
    void SetSplitLineColor(uint32_t color);

    void SetOverscrollBarColor(uint32_t color);

    int32_t GetScrollingValue();
    int32_t GetScrollMaxValue();
    virtual Component* GetElement(uint32_t index);
    virtual Component* GetElement(const char* id);
    virtual Component* GetLastElement();

    //Touch
    virtual Touch::TouchResponse ProcessTouch(const Touch& tp, int32_t ParentX, int32_t ParentY, int32_t modificator = 0);
    void EnableScrolling();
    void DisableScrolling();
    void SetScrolling(bool enable);

    void EnableOverscrollBar();
    void DisableOverscrollBar();
    void SetOverscrollBar(bool enable);
    void SetOverscrollBarSize(int32_t size);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);
    virtual void SetSize(int32_t width, int32_t height);
    virtual void ScreenInit();

    virtual void CheckPlacement();

    void SetScrollIndicatorColor(uint32_t color);

protected:
    int32_t Scroll, ScrollMax, ScrollChange, prevDeltaY, prevDeltaX, itemsHeight;
    int8_t animation;
    float dSpeed;
    uint32_t ElementColor, SplitLineColor;
    typedef vector<AbstractButton*> Elements_vec;
    Elements_vec Elements;
    uint64_t scrollingTimeStamp;
    bool scrollingEnabled, overscrollBarEnabled;
    int32_t overscrollBarSize, currentOverscrollBarSize;
    uint32_t overscrollBarColor;
    uint8_t currentSample;
    int32_t speedSamples[3];
    uint64_t scrollIndicatorTimestamp;
    uint32_t scrollIndicatorColor;
    uint8_t scrollIndicatorOpacity;
};

} /* namespace guilib */

#endif /* GUI_LIB_SRC_VERTICALSCROLLVIEW_H_ */
