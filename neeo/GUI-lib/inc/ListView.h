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
 * Created on: 2014-09-10
 *     Author: wstrozynski@antmicro.com
 *             dkochmanski@antmicro.com
 *
 */

#ifndef GUILISTVIEW_H_
#define GUILISTVIEW_H_

#include "VerticalScrollView.h"
#include "ListItem.h"
#include "stl.h"
#include "Painter.h"

namespace guilib {

class ListView : public VerticalScrollView {
public:
    ListView()
        : VerticalScrollView() {
        SplitLineColor = COLOR_ARGB8888_BLACK;
        overscrollBarEnabled = true;
    };

    ListView(int32_t x, int32_t y, int32_t width, int32_t height)
        : VerticalScrollView(x, y, width, height) {
        SplitLineColor = COLOR_ARGB8888_BLACK;
        overscrollBarEnabled = true;
    };

    virtual ~ListView();

    virtual void AddElement(ListItem *item);

    //XML operations
    void ClearList();
    bool AddToList(Manager* man, string& listContent);

};

} /* namespace guilib */

#endif /* GUILISTVIEW_H_ */


