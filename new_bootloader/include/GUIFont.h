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
 * Created on: 2014-11-14
 *     Author: wstrozynski@antmicro.com 
 *
 */

#ifndef AMGNEWFONT_H_
#define AMGNEWFONT_H_

#include "stl.h"
#include <stdint.h>

typedef struct {
        void* self;
        uint16_t BasicLength;
        uint16_t CharHeight;
        uint8_t* (*_GetCharData)(const void* self,uint16_t code);
        int8_t (*_GetKerning)(const void* self,int8_t leftCode, int8_t rightCode);
        uint16_t (*_GetCharWidth)(const void* self,uint16_t code);
        uint16_t (*_GetWidth)(const void* self,const char* text);
}fontmethods_t;

class GUIFont {
public:

    struct unicode_character{
        uint32_t code;
        int32_t  length;
    };

    struct kerning_entry {
        uint8_t left;
        uint8_t right;
        int8_t space;
    };

    struct __attribute__((packed)) map_entry {
        uint16_t code;
        uint32_t offset;
    };

    GUIFont()
        : CharacterEntries(0), CharHeight(0), FontHeight(0), BasicLength(0),
          CharMap(0), CharData(0), Widths(0), KerningMap(0), KerningData(0), KerningEntries(0) {};

    GUIFont(const uint16_t* FontData);
    GUIFont(const char * fpath);
    GUIFont(const GUIFont& Obj)
        : CharacterEntries(Obj.CharacterEntries), CharHeight(Obj.CharHeight), FontHeight(Obj.FontHeight),
          BasicLength(Obj.BasicLength), CharMap(Obj.CharMap), CharData(Obj.CharData), Widths(Obj.Widths), KerningMap(Obj.KerningMap),
          KerningData(Obj.KerningData), KerningEntries(Obj.KerningEntries){
        CodeMap = Obj.CodeMap; //Fixme Is this correct?
    };

    virtual ~GUIFont();

    GUIFont& operator=(const GUIFont& Obj);

    uint16_t GetCharEntries() const;
    uint16_t GetFontHeight() const;
    uint16_t GetHeight() const;
    uint16_t GetBasicLength() const;
    uint16_t GetKerningEntries() const;

    uint8_t* GetCharData(uint32_t code) const;
    uint16_t GetCharWidth(uint32_t code) const;
    int8_t   GetKerning(uint32_t leftCode, uint32_t rightCode) const;

    uint16_t GetWidth(const char* text) const;

    static unicode_character getUnicodeCharacter(const char *in);

protected:
    typedef map<uint16_t, uint16_t> Map;
    Map CodeMap;
    uint16_t CharacterEntries;  //Number of characters
    uint16_t CharHeight;        //
    uint16_t FontHeight;        //
    uint16_t BasicLength;       //Standard space
    map_entry *CharMap;         //Char map location
    uint8_t  *CharData;         //Char data location
    uint8_t  *Widths;           //Widths map location
    uint16_t *KerningMap;       //Kerning Map location (lookup table)
    kerning_entry *KerningData; //Kerning Data location
    uint16_t KerningEntries;    //Number of kerning entries

    uint16_t SwapBytes(uint16_t data) const;
    uint32_t swap_uint32( uint32_t val ) const;
    void Init(const uint16_t* data);
};

#endif /* AMGFONT_H_ */
