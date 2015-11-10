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
 * Created on: 2015-4-16
 *     Author: pzierhoffer@antmicro.com 
 *
 */

#ifndef GUICLOCK_H_
#define GUICLOCK_H_

#include "stl.h"
#include <time.h>

#include "GUIFont.h"
#include "Label.h"
#include "tinyxml2.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class Clock : public Label{
public:
    Clock()
        : Label(), isRunning(false), last_current_time(0) {
    };
    Clock(int32_t x, int32_t y, int32_t width, int32_t height)
        : Label(x, y, width, height), isRunning(false), last_current_time(0) {
    };
    Clock(int32_t x, int32_t y, int32_t width, int32_t height, TextHorizontalAlignment alignment)
        : Label(x, y, width, height, "", alignment), isRunning(false), last_current_time(0) {
    };

    static Clock* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

    void Start();
    void Stop();
    bool IsRunning();

private:
    bool isRunning;
    time_t last_current_time;
    static const int bufferSize = 10;
};

} /* namespace guilib */

#endif /* GUICLOCK_H_ */
