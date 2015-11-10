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
 * Created on: 6 Oct 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUI_LIB_SRC_CONTENTMANAGER_H_
#define GUI_LIB_SRC_CONTENTMANAGER_H_

#include "stl.h"

namespace guilib {
class Image;
class ImageContent;

class ContentManager {
public:
    ContentManager()
    : RequestContentCallback(NULL), CancelRequestCallback(NULL){};
    virtual ~ContentManager();

    typedef map<string, ImageContent*> ImageContentMap;
    typedef map<Image*, string> ImageBindingMap;
    typedef ImageBindingMap::iterator ImageBindingIterator;
    typedef vector<ImageBindingIterator> BindingsToRemove;
    typedef vector<string> PendingRequestsVector;

    typedef void (*ContentCallback)(const string contentName);

    void AddInternalImageContent(string& name, ImageContent* ic);
    void AddExternalImageContent(string& name, ImageContent* ic);
    void BindImageContentToImage(const string& contentName, Image* image);

    bool TryToFindInInternalContent(const string& contentName, Image* image);
    bool TryToFindInExternalContent(const string& contentName, Image* image);

    void RequestBinding(Image* image);
    void CancelRequest(Image* image); //proposition

    void SetRequestCallback(ContentCallback requestCallback);
    void SetCancelRequestCallback(ContentCallback cancelRequestCallback);

private:
    ImageContentMap InternalImageContainer; //Can not be deleted
    ImageContentMap ExternalImageContainer; //Can be deleted
    ImageBindingMap MissingContent;
    PendingRequestsVector PendingRequests;
    ContentCallback RequestContentCallback, CancelRequestCallback;

    void UpdateContent(string& name, ImageContent* ic);
};

} /* namespace guilib */

#endif /* GUI_LIB_SRC_CONTENTMANAGER_H_ */
