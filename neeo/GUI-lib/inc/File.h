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
 * Created on: 2015-04-15
 *     Author: mholenko@antmicro.com
 *
 */

#ifndef FILE_H_
#define FILE_H_

#include <stdint.h>

class File {
public:
    File(const char* path);

    int32_t GetSize();
    bool ReadToBuffer(uint8_t* &buffer);
    bool WriteFromBuffer(uint8_t* &buffer, uint32_t size);
    bool HasExtension(const char* ext);

private:
    const char* path;
    bool readOnly;
    bool gzipped;
};

#endif
