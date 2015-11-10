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
 * Created on: 2015-03-19
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUIEVENT_H_
#define GUIEVENT_H_

#include <stdint.h>
#include "stl.h"

namespace guilib {


class Event {
public:

    typedef vector<string> ArgVector;
    typedef void (*CallbackPtr)(void* senderPtr, const Event::ArgVector&);

    Event()
    : senderPtr(0), eventCallback(0){

    }
    Event(CallbackPtr callback, const ArgVector& argVector)
    : senderPtr(0), eventCallback(callback){
        argVec = argVector;
    }
    Event(void* sender, CallbackPtr callback, const ArgVector& argVector)
    : senderPtr(sender), eventCallback(callback){
        argVec = argVector;
    }
    virtual ~Event();


    void Trigger();
    void SetSenderPtr(void* sender);
    void SetCallback(CallbackPtr Callback, const ArgVector& argVector);

    bool IsSet();

private:
    ArgVector argVec;
    void * senderPtr;
    CallbackPtr eventCallback;

};

} /* namespace guilib */

#endif /* GUIEVENT_H_ */
