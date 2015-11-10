/*
 * Painter.h
 *
 *  Created on: 22 Apr 2015
 *      Author: ws
 */

#ifndef GUI_LIB_SRC_PAINTER_H_
#define GUI_LIB_SRC_PAINTER_H_

#include <stdint.h>
#include "stl.h"
#include "GUIFont.h"

namespace guilib {
class Image;
class ContentManager;

typedef struct {
    uintptr_t data;
    uint32_t pixel_format;
    uint8_t bpp;
} layer_t;

typedef struct {
    int32_t y_position;
    int32_t height;
} background_block;

class Painter {
public:
    Painter()
    : XSize(0), YSize(0), BottomSwapper(0), TopSwapper(0), activeLayer(0), BackgroundImage(0), contentManager(0),
      is_rotated(false), active_layer(0) { };
    virtual ~Painter();

    void SetDisplaySize(int32_t x, int32_t y);
    void CreateFramebuffersCollection(uint8_t FrontBPP, uint8_t BackBPP);
    void InitFramebuffersCollection();
    void SetBackgroundImage(Image* image);

    void SetContentManager(ContentManager* cm);
    ContentManager* GetContentManager();

    uint32_t GetXSize() const;
    uint32_t GetYSize() const;
    uint8_t GetBottomSwapperValue() const; //temporary

    uintptr_t GetBackBuffer(int id) const;
    uintptr_t GetVisibleBackBuffer() const;
    uintptr_t GetDrawableBackBuffer() const;
    uintptr_t GetVisibleFrontBuffer() const;
    uintptr_t GetDrawableFrontBuffer() const;

    void FlipTopBuffers();
    void FlipBottomBuffers();
    void SynchronizedTopBuffersFlip();
    void SynchronizedBottomBuffersFlip();
    void SynchronizedBuffersFlip();
    void SetBackBuffer(int id);
    void SelectLayer(uint32_t id);

    uint32_t GetActiveLayer() const;
    uintptr_t GetActiveLayerPointer() const;

    void FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint32_t color) const;
    void DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint32_t color) const;


    void DrawVLine(int32_t Xpos, int32_t Ypos, uint16_t Length, uint32_t text_color) const;
    void DrawPixel (uint32_t Xpos, uint32_t Ypos, uint32_t RGB_Code) const;
    uint32_t ReadPixel(uint32_t Xpos, uint32_t Ypos) const;


    void SetLayerAddress(uint32_t id, uint32_t display);

    void DMA_Operation(uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines,
        uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor = 0) const;

    void DMA_Fill(uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t outOffset, uint32_t color_index, uint32_t pixel_format) const;

    void DMA_Move(uintptr_t fb_src, uintptr_t fb_dst,
                  int32_t x_src, int32_t y_src,
                  int32_t x_dst, int32_t y_dst,
                  int32_t width, int32_t height,
		  uint32_t src_pixel_format, uint32_t dst_pixel_format) const;

    void DMA_MoveImage(uintptr_t img_src, uintptr_t fb_dst,
                  int32_t x_src, int32_t y_src, int32_t x_dst, int32_t y_dst,
                  int32_t width, int32_t height, int32_t totalImageWidth, int32_t totalImageHeight,
                  uint32_t activeFrame, uint32_t frames, uint32_t inPixelFormat, uint32_t outPixelFormat, bool hasAlpha) const;

    void DMA_MoveFont(uintptr_t font_src, uintptr_t font_dst,
                  int32_t x_src, int32_t y_src, int32_t x_dst, int32_t y_dst,
                  int32_t width, int32_t height, int32_t fontWidth, int32_t fontHeight, uint32_t outPixelFormat, uint32_t fontColor) const;

    bool IsRotated() const;

    void DrawHLine(int32_t Xpos, int32_t Ypos, uint16_t Length, uint32_t text_color) const;

    void DisplayAntialiasedStringAt(const GUIFont* Font, int16_t Xpos, int16_t Ypos, const char *Text, uint32_t text_color) const;
    void DisplayAntialiasedChar(const GUIFont* Font, uint16_t Xpos, uint16_t Ypos, uint32_t index, uint32_t text_color) const;
    void DrawAntialiasedChar(const GUIFont* Font, int16_t Xpos, int16_t Ypos, const uint8_t *c, uint32_t Index, uint32_t text_color) const;
    void DrawAntialiasedCharInBaund(const GUIFont* Font, int16_t Xpos, int16_t Ypos,int16_t ParentX, int16_t ParentY, int16_t ParentWidth, int16_t ParentHeight, const uint8_t *c, uint32_t Index, uint32_t text_color) const;
    void DisplayAntialiasedCharInBaund(const GUIFont* Font, int16_t Xpos, int16_t Ypos,int16_t ParentX, int16_t ParentY, int16_t ParentWidth, int16_t ParentHeight, uint32_t index, uint32_t text_color) const;
    void DisplayBoundedAntialiasedStringAt(const GUIFont* Font, int16_t Xpos, int16_t Ypos, int16_t ParentX, int16_t ParentY, int16_t ParentWidth, int16_t ParentHeight, const uint8_t *Text, uint32_t text_color) const;
    uint32_t GetLayerPixelFormat(uint32_t id) const;
    uint32_t GetLayerBytesPerPixel(uint32_t id) const;
    uint32_t GetLayerDisplayPixelFormat(uint32_t id) const;
    uint32_t GetLayerDisplayBytesPerPixel(uint32_t id) const;
    void SetLayerTransparency(uint32_t id, uint8_t value);
    void SetBackgroundColour(uint32_t colour);
    void SetRotation(bool rotate90);
    int32_t GetDisplayWidth() const;
    int32_t GetDisplayHeight() const;
    void FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col) const;
    void DrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col) const;

    //Background blocks
    typedef vector<background_block> background_block_vector;
    background_block_vector bblocks;
    void AddBackgroundBlock(int32_t y_position, int32_t height, uint32_t backgroundColor);
    void DMA_TransferToFramebuffer(int32_t y_position, int32_t height, bool with_background);

    //Useful methods.
    static bool colorHasTransparency(uint32_t color);

protected:
    layer_t frontLayerPointers[2];
    layer_t backLayerPointers[3];
    uint32_t XSize, YSize;
    uint8_t BottomSwapper, TopSwapper;
    uint32_t activeLayer;
    Image* BackgroundImage;
    ContentManager* contentManager;
    bool is_rotated;
    uint8_t active_layer;
    uint16_t Power(const int8_t a, const int8_t b) const;
};

} /* namespace stmgui */

#endif /* GUI_LIB_SRC_PAINTER_H_ */
