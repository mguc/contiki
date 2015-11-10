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
 * Created on: 2015-03-13
 *     Author: kkruczynski@antmicro.com
 *
 */

#ifndef GUIIMAGECONTENT_H_
#define GUIIMAGECONTENT_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "GUILib.h"
#include "File.h"
#include "misc.h"
#include <Painter.h>
#include "Definitions.h"

namespace guilib {

enum ImageType { IMAGE_TYPE_RAW, IMAGE_TYPE_PNG };
enum DitherMode { DITHER_NONE, DITHER_FLOYD_STEINBERG, DITHER_INDEXED };

class ImageContent {
public:

    struct FromPNG {
        FromPNG(uint8_t *data, uint32_t dataLength, uint16_t numberOfFrames, bool alpha, DitherMode dither = DITHER_NONE)
            : data(data), dataLength(dataLength), numberOfFrames(numberOfFrames), alpha(alpha), dither(dither), initialized(true) {};

        FromPNG(const char* fpath, uint16_t numberOfFrames, bool alpha, DitherMode dither = DITHER_NONE)
            : data(NULL), dataLength(0), numberOfFrames(numberOfFrames), alpha(alpha), dither(dither), initialized(false)
        {
            File f(fpath);
            int32_t fileSize = f.GetSize();
            if (fileSize == -1 || !f.HasExtension("png"))
            {
                GUILib::Log("[ERROR] GUIImageContent: Could not load PNG image from file %s", fpath);
                return;
            }

            dataLength = fileSize;
            data = (uint8_t*) GUILib::Callbacks()->malloc(dataLength);
            initialized = f.ReadToBuffer(data);
        };

        uint8_t *data;
        uint32_t dataLength;
        uint16_t numberOfFrames;
        bool alpha;
        DitherMode dither;

        bool initialized;
    };

    struct FromRAW : FromPNG {
        FromRAW(uint8_t* data, uint32_t width, uint32_t height, uint16_t numberOfFrames, uint32_t colorFormat = COLOR_FORMAT_ARGB1555)
            : FromPNG(data, 0, numberOfFrames, false), width(width), height(height), colorFormat(colorFormat) {};

        FromRAW(const char* fpath, uint32_t width, uint32_t height, uint16_t numberOfFrames, uint32_t colorFormat = COLOR_FORMAT_ARGB1555)
            : FromPNG(NULL, 0, numberOfFrames, false, DITHER_NONE), width(width), height(height), colorFormat(colorFormat)
        {
            initialized = false;
            File f(fpath);
            int32_t fileSize = f.GetSize();
            if (fileSize == -1 || !f.HasExtension("raw"))
            {
                GUILib::Log("[ERROR] GUIImageContent: Could not load RAW image from file %s", fpath);
                return;
            }

            dataLength = fileSize;
            data = (uint8_t*) GUILib::Callbacks()->malloc(dataLength);
            initialized = f.ReadToBuffer(data);
        };

        uint32_t width;
        uint32_t height;
        uint32_t colorFormat;
    };

    ImageContent(const FromPNG &fromPNG)
    {
        if (fromPNG.initialized)
        {
            Init(fromPNG.data, fromPNG.dataLength, 0, 0, 0, fromPNG.numberOfFrames, fromPNG.alpha, fromPNG.dither, IMAGE_TYPE_PNG);
            GUILib::Callbacks()->free(fromPNG.data);
        }
        else
        {
            data = NULL;
        }
    }

    ImageContent(const FromRAW &fromRAW)
    {
        if (fromRAW.initialized)
        {
            Init(fromRAW.data, fromRAW.dataLength, fromRAW.colorFormat, fromRAW.width, fromRAW.height, fromRAW.numberOfFrames, false, DITHER_NONE, IMAGE_TYPE_RAW);
        }
        else
        {
            data = NULL;
        }
    }

    ImageContent(const ImageContent& content);

    ImageContent& operator=(const ImageContent& other);

    virtual ~ImageContent();

    uint8_t* GetData() const {
        return data;
    }

    uint32_t GetDataLength() const {
        return width * height * numberOfFrames * GetBytesPerPixel();
    }

    uint16_t GetNumberOfFrames() const {
        return numberOfFrames;
    }

    uint32_t GetWidth() const {
        return width;
    }

    uint32_t GetHeight() const {
        return height;
    }

    uint32_t GetColorFormat() const {
        return colorFormat;
    }

    uint32_t GetBytesPerPixel() const {
        return pixelFormatToBPP(colorFormat);
    }

    uint32_t GetNumberOfLines() const {
        return lines;
    }

    uint32_t GetPixelsPerLine() const {
        return pixelsPerLine;
    }

    bool HasAlphaChannel() const {
        return hasAlpha;
    }

    bool IsRotated() {
        return rotated;
    }

    bool IsEmpty()
    {
        return !data;
    }

    void Rotate90();

private:

    uint8_t *data;
    uint16_t numberOfFrames;
    uint32_t width, height, lines, pixelsPerLine;
    ImageType imageType;
    uint32_t colorFormat;
    bool rotated, hasAlpha;

    void Init(uint8_t *data, uint32_t dataLength, uint32_t colorFormat, uint32_t width, uint32_t height, uint16_t numberOfFrames,
            bool alpha, DitherMode dither, ImageType imageType);
    uint32_t XYToOffset(uint32_t x, uint32_t y, uint32_t byteCount, uint32_t bytesPerPixel, uint32_t wholeImageWidth);

};

} /* namespace guilib */

#endif /* GUIIMAGECONTENT_H_ */
