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
 * Created on: 2014-12-08
 *     Author: wstrozynski@antmicro.com
 *             dkochmanski@antmicro.com
 *
 */

#ifndef GUIPANEL_H_
#define GUIPANEL_H_

#include "Container.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class Panel : public Container {

public:
    Panel()
        : Container() {};
    Panel(int32_t x, int32_t y, int32_t width, int32_t height)
        : Container(x, y, width, height) {};
    Panel(const Panel& Obj)
        : Container(Obj) {};

    virtual ~Panel();

    static Panel* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual int32_t GetWidth();
    virtual int32_t GetHeight();

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);
protected:
};

} /* namespace guilib */

#endif
