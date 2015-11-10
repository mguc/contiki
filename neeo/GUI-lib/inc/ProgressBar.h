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
 * Created on: 12 Aug 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef INC_PROGRESSBAR_H_
#define INC_PROGRESSBAR_H_

#include "Component.h"
#include "stl.h"
#include "Painter.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class ProgressBar : public Component {
public:
    ProgressBar()
    : Component(), ProgressValue(0) {}
    ProgressBar(int32_t x, int32_t y, int32_t width, int32_t height)
    : Component(x, y, width, height), ProgressValue(0) {}
    virtual ~ProgressBar();

    void SetProgressValue(int value);
    int32_t GetProgressValue();

    static ProgressBar* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

private:
    int ProgressValue;
};

} /* namespace guilib */

#endif /* INC_PROGRESSBAR_H_ */
