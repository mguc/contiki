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
 * Created on: 2014-09-01
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef GUIIMAGE_H_
#define GUIIMAGE_H_

#include "Component.h"
#include "ImageContent.h"
#include "stl.h"
#include "Painter.h"
#include "tinyxml2.h"
#include "ContentManager.h"

using namespace tinyxml2;

namespace guilib {
class Manager;

class Image : public Component {
public:
    Image()
    : Component(), ActiveFrame(0), Content(NULL), bindingRegistered(false), imageContentRequested(false) {};

    Image(ImageContent* content, uint32_t x = 0, uint32_t y = 0, uint32_t activeFrame = 0)
    : Component(x, y, 0, 0), ActiveFrame(activeFrame), Content(content), bindingRegistered(false), imageContentRequested(false) {
        if(content != NULL) {
            Width = content->GetWidth(); Height = content->GetHeight();
        }
    };

    virtual ~Image() {};

    void SetActiveFrame(uint32_t activeFrame);

    uint32_t GetActiveFrame();

    ImageContent* GetContent() {
        return Content;
    }

    //Deletes the old imageContent and set the new one
    void UpdateImageContent(ImageContent* content);
    //Changes the pointer only
    void ChangeImageContent(ImageContent* content);
    void DeleteImageContent();
    void CleanImageContent();

    uint8_t* GetContentData() const {
        if(Content) return Content->GetData();
        else return 0;
    }

    uint32_t GetContentBytesPerPixel() const {
        if(Content) return Content->GetBytesPerPixel();
        else return 0;
    }

    uint32_t GetContentColorFormat() const {
        if(Content) return Content->GetColorFormat();
        else return 0;
    }

    bool GetContentAlpha() const {
        if(Content) return Content->HasAlphaChannel();
        else return false;
    }

    bool IsEmpty() const;
    bool IsImageContentPending() const;

    static Image* BuildFromXml(Manager* man, XMLElement* xmlElement);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

private:
    uint32_t ActiveFrame;
    ImageContent* Content;

    bool bindingRegistered; //Binding request has already been sent
    bool imageContentRequested; //Waiting for content to be fulfilled by ContentManager

    friend void ContentManager::BindImageContentToImage(const string& contentName, Image* image);
    friend void ContentManager::CancelRequest(Image* image);
    friend void ContentManager::RequestBinding(Image* image);

};

} /* namespace guilib */

#endif /* GUIIMAGE_H_ */
