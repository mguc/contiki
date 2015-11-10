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
 * Created on: 2015-02-13
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef ANTMICRO_GUI_GUI_LIB_INC_GUITOUCH_H_
#define ANTMICRO_GUI_GUI_LIB_INC_GUITOUCH_H_

#include <stdint.h>

namespace guilib {

class Touch {
public:

    enum TouchResponse {
        TouchNA = 0,
        TouchHandled,
        TouchReleased
    };

    enum TouchState {
        Idle = 0,
        Pressed,
        Released,
        Moving
    };

    Touch()
    : state(Idle), startX(0), startY(0), deltaX(0), deltaY(0) {
    }
    Touch(TouchState initState, int32_t startX, int32_t startY)
    : state(initState), startX(startX), startY(startY), deltaX(0), deltaY(0) {
    }

    virtual ~Touch();

    void SetStartPosition(int32_t x, int32_t y);
    void SetCurrentPosition(int32_t x, int32_t y);
    void SetState(Touch::TouchState state);

    int32_t GetDeltaX() const;
    int32_t GetDeltaY() const;
    int32_t GetCurrentX() const;
    int32_t GetCurrentY() const;
    int32_t GetStartX() const;
    int32_t GetStartY() const;
    TouchState GetState() const;

private:
    TouchState state;
    int32_t startX, startY, deltaX, deltaY;


};

} /* namespace guilib */

#endif /* ANTMICRO_GUI_GUI_LIB_INC_GUITOUCH_H_ */
