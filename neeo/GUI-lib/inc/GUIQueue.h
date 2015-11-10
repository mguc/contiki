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
 * Created on: 2015-08-07
 *     Author: mholenko@antmicro.com
 *
 */

#ifndef GUIEVENTQUEUE_H_
#define GUIEVENTQUEUE_H_

#include <stdint.h>
#include <pthread.h>

#include "stl.h"

namespace guilib {

template <class T>
class GUIQueue {

public:
    GUIQueue();
    void push(T* element);
    T* pop();

private:
    pthread_mutex_t mutex;
    queue<T*> elementsQueue;
};

template <class T>
GUIQueue<T>::GUIQueue() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&mutex, &attr);
}

template <class T>
void GUIQueue<T>::push(T* element) {
    if(pthread_mutex_lock(&mutex) != 0) {
        return;
    }

    elementsQueue.push(element);

    pthread_mutex_unlock(&mutex);
}

template <class T>
T* GUIQueue<T>::pop() {
    if(pthread_mutex_lock(&mutex) != 0) {
        return NULL;
    }

    T* result;
    if (elementsQueue.empty())
    {
        result = NULL;
    }
    else
    {
        result = elementsQueue.front();
        elementsQueue.pop();
    }

    pthread_mutex_unlock(&mutex);
    return result;
}

}

#endif
