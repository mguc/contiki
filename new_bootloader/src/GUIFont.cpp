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

#include "GUIFont.h"
#include "romfs.h"
#include <stdio.h>
const int spaceMultiplicator = 2; // TODO Not very good idea.

GUIFont::~GUIFont() {
}

GUIFont::GUIFont(const uint16_t* FontData) {
    Init(FontData);
}

GUIFont::GUIFont(const char* fpath) {

    long int length;
    uint8_t* buffer = romfs_allocate_and_get_file_content(fpath, false, &length);

    if (length == -1) {
    } else {
	    Init((uint16_t*) buffer);
    }
}

void GUIFont::Init(const uint16_t* FontData) {
    CharacterEntries = (SwapBytes(FontData[0]));
    CharHeight = (SwapBytes(FontData[1]));
    FontHeight = (SwapBytes(FontData[2]));
    BasicLength = (SwapBytes(FontData[3]));
    uint16_t CharMapOffset((uint32_t)SwapBytes(FontData[4]));
    uint32_t CharWidthsOffset((uint32_t)swap_uint32((uint32_t)FontData[5] | (uint32_t)FontData[6]<<16));
    uint32_t KerningMapOffset((uint32_t)swap_uint32((uint32_t)FontData[7] | (uint32_t)FontData[8]<<16));
    KerningEntries = SwapBytes(FontData[9]);

    CharMap = (map_entry*)((uintptr_t)FontData + CharMapOffset);
    Widths = (uint8_t*)((uintptr_t)FontData + CharWidthsOffset);
    KerningMap = (uint16_t*)((uintptr_t)FontData + KerningMapOffset);

    CharData = (uint8_t*)((uintptr_t)KerningMap + (uint32_t)(2*CharacterEntries));
    KerningData = (kerning_entry*)((uintptr_t)Widths + (uint32_t)(CharacterEntries));

    //Filling map
    for(int i=0; i<CharacterEntries; i++){
        CodeMap[SwapBytes(CharMap[i].code)] = i;
    }
}

GUIFont& GUIFont::operator =(const GUIFont& Obj) {
    if(this != &Obj){
        CodeMap = Obj.CodeMap;
        CharacterEntries = Obj.CharacterEntries;
        CharHeight = Obj.CharHeight;
        FontHeight = Obj.FontHeight;
        BasicLength = Obj.BasicLength;
        CharMap = Obj.CharMap;
        CharData = Obj.CharData;
        Widths = Obj.Widths;
        KerningMap = Obj.KerningMap;
        KerningData = Obj.KerningData;
        KerningEntries = Obj.KerningEntries;
    }
    return *this;
}

uint16_t GUIFont::GetHeight() const {
    return CharHeight;
}

uint16_t GUIFont::GetKerningEntries() const {
    return KerningEntries;
}

uint8_t* GUIFont::GetCharData(uint32_t code) const {
    Map::const_iterator searchCode = CodeMap.find(code);
    if(searchCode == CodeMap.end()){ //If Looking for '_'
        searchCode = CodeMap.find(0x5f);
        if(searchCode == CodeMap.end()) return 0; //Not available
    }

    return (uint8_t*)((uintptr_t)CharData + (uintptr_t)swap_uint32(CharMap[searchCode->second].offset));
}

uint16_t GUIFont::GetCharWidth(const uint32_t code) const {
    Map::const_iterator searchCode = CodeMap.find(code);
    if(searchCode == CodeMap.end()){ //If Looking for '_'
        searchCode = CodeMap.find(0x5f);
        if(searchCode == CodeMap.end()) return BasicLength*3; //Not available
    }

    return Widths[searchCode->second];
}

// based on MIT-licensed Haiku-OS, taken from: UdfString.cpp
GUIFont::unicode_character GUIFont::getUnicodeCharacter(const char* in) {
    unicode_character uchar;
    if (!in){
        uchar.code = 0;
        uchar.length = 0;
        return uchar;
    }

    uint8_t *bytes = (uint8_t *)in;
    if (!bytes){
        uchar.code = 0;
        uchar.length = 0;
        return uchar;
    }

    int32_t length;
    uint8_t mask = 0x1f;

    switch (bytes[0] & 0xf0) {
            case 0xc0:
            case 0xd0:  length = 2; break;
            case 0xe0:  length = 3; break;
            case 0xf0:  mask = 0x0f; length = 4; break;
            default:
                // valid 1-byte character
                // and invalid characters
                (in)++;
                uchar.length = 1;
                uchar.code = (uint32_t)bytes[0];
                return uchar;
    }
    uint32_t c = bytes[0] & mask;
    int32_t i = 1;
    for (;i < length && (bytes[i] & 0x80) > 0;i++)
            c = (c << 6) | (bytes[i] & 0x3f);
    if (i < length) {
            // invalid character
            uchar.length = 1;
            uchar.code = (uint32_t)bytes[0];
            return uchar;
    }
    uchar.length = length;
    uchar.code = (uint32_t)c;
    return uchar;
}

int8_t GUIFont::GetKerning(uint32_t leftCode, uint32_t rightCode) const {

    Map::const_iterator searchLeftCode = CodeMap.find(leftCode);
    if(searchLeftCode == CodeMap.end()) return BasicLength;

    Map::const_iterator searchRightCode = CodeMap.find(rightCode);
    if(searchRightCode == CodeMap.end()) return BasicLength;

    uint16_t
        leftIndex = searchLeftCode->second,
        rightIndex = searchRightCode->second;

    uint16_t kerningOffset = 0;
    int8_t KerningValue = BasicLength;

    if(KerningEntries == 0) return KerningValue;
    else{
        if(KerningMap) kerningOffset = SwapBytes(KerningMap[leftIndex]);

        if(kerningOffset == 0xffff) {return KerningValue;}
        else{ //Find kerning

            kerning_entry* CharKerning = (kerning_entry*)((unsigned long)KerningData+(uint32_t)kerningOffset);

            for(int i = 0; CharKerning[i].left == leftIndex; i++){
                if(CharKerning[i].right == rightIndex) {KerningValue += CharKerning[i].space; break;}
            }
            return KerningValue;
        }
    }
}

uint16_t GUIFont::GetWidth(const char* text) const {

    int32_t size = 0;

    while(*text != 0)
    {
        uint32_t charCode = 0, nextCharCode = 0;
        GUIFont::unicode_character uchar, next_uchar;
        int32_t length=1, charWidth=0;

        if(*text != 0x20) { //Not a space
            if((unsigned char)(*text) > 0x7F) { //Unicode
                uchar = getUnicodeCharacter(text);
                length = uchar.length;
                charCode = uchar.code;
                if(*(text+length) != 0){
                    next_uchar = getUnicodeCharacter(text+length);
                    nextCharCode = next_uchar.code;
                }
            } else { //UTF8
                charCode = *text;
                if((*(text+length)) != 0){
                    if((*(text+length)) <= 0x7f){
                        nextCharCode = *(text+length);
                    } else { //Next character is a unicode character
                        next_uchar = getUnicodeCharacter(text+length);
                        nextCharCode = next_uchar.code;
                    }
                }
            }

            charWidth = GetCharWidth(charCode);
        } else { //Space
            charCode = *text;
            nextCharCode = *(text+length);
            charWidth = (BasicLength)*spaceMultiplicator;
        }

        size += charWidth;
        if(nextCharCode) size += GetKerning(charCode, nextCharCode);
        text+= length;
    }

    return size;
}

uint16_t GUIFont::GetCharEntries() const{
    return CharacterEntries;
}

uint16_t GUIFont::GetFontHeight() const{
    return FontHeight;
}

uint16_t GUIFont::GetBasicLength() const{
    return BasicLength;
}

uint16_t GUIFont::SwapBytes(uint16_t data) const {
    return (data>>8) | (data<<8);
}

uint32_t GUIFont::swap_uint32( uint32_t val ) const
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}
