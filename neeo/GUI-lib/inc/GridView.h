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

#ifndef GUIGRIDVIEW_H_
#define GUIGRIDVIEW_H_

#include "VerticalScrollView.h"
#include "GridRow.h"
#include "stl.h"
#include "Painter.h"

namespace guilib {

class GridView : public VerticalScrollView {

public:
    GridView()
        : VerticalScrollView(), ElementWidth(0), ElementHeight(0), VerticalOffset(0) {
    };

    GridView(int32_t x, int32_t y, int32_t width, int32_t height)
        : VerticalScrollView(x, y, width, height), ElementWidth(0), ElementHeight(0), VerticalOffset(0) {
    };
    virtual ~GridView();

    void SetGridParams(uint32_t width, uint32_t height, uint32_t verticalOffset);
    void AddElement(GridRow *item);

    virtual void SetBackgroundColor(uint32_t color);

    virtual void ScreenInit() {}
    
protected:
    uint32_t ElementWidth, ElementHeight, VerticalOffset;

};

} /* namespace guilib */

#endif /* GUIGRIDVIEW_H_ */
