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
 * Created on: 2015-01-19
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef GUI_LIB_GUIICO_H_
#define GUI_LIB_GUIICO_H_

#include "Component.h"
#include "GUIFont.h"
#include "Painter.h"

namespace guilib {

class Ico : public Component {
protected:
    int32_t IcoChar;
    GUIFont const * IcoFont;

public:
    Ico()
    : Component(), IcoChar(-1), IcoFont(0){
    }
    Ico(uint32_t width, uint32_t height, int32_t charCode, GUIFont* IcoFont)
    : Component(0,0,width, height), IcoChar(charCode), IcoFont(IcoFont){
    }
    Ico(int32_t charCode, GUIFont* IcoFont);

    virtual ~Ico();

    int32_t GetIcoChar() const;
    void SetIcoChar(int32_t icoChar);
    GUIFont const * GetIcoFont() const;
    void SetIcoFont(GUIFont const * icoFont);

    virtual void Draw(Painter& painter, int32_t ParentX, int32_t ParentY, int32_t ParentWidth, int32_t ParentHeight);

private:
    void AdjustSize();
};

} /* namespace guilib */

#endif /* GUI_LIB_GUIICO_H_ */
