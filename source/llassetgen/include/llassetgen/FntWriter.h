#pragma once

#include <set>
#include <string>
#include <vector>

#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

#include <ft2build.h>
#include <llassetgen/packing/Types.h>
#include FT_FREETYPE_H

namespace llassetgen {
    struct Info {
        std::string face;
        int size;
        bool isBold;
        bool isItalic;
        std::string charset;
        bool useUnicode;
        /*
        float stretchH;
        bool useSmoothing;
        int supersamplingLevel;
        */
        struct padding {
                float left;
                float right;
                float up;
                float down;
        } padding;
        struct spacing {
                float horiz;
                float vert;
        } spacing;
        /*
        float outlineThickness;
        */
    };

    struct Common {
        int lineHeight;
        int base;
        int scaleW;
        int scaleH;
        int pages;
        bool isPacked;
        int ascent;
        int descent;
    };

    struct CharInfo {
        int id;
        int x;
        int y;
        int width;
        int height;
        float xOffset;
        float yOffset;
        float xAdvance;
        int page;
        uint8_t chnl;
    };

    struct KerningInfo {
        int firstId;
        int secondId;
        float kerning;
    };

    class LLASSETGEN_API FntWriter {
       public:
        FntWriter(FT_Face face, std::string faceName, unsigned int fontSize, float scalingFactor, float padding);
        void readFont(std::set<FT_ULong>::iterator charcodesBegin, std::set<FT_ULong>::iterator charcodesEnd);
        void setAtlasProperties(Vec2<PackingSizeType> size);
        void saveFnt(std::string filepath);
        void setCharInfo(FT_ULong charcode, Rect<PackingSizeType> charArea);
        bool setCharInfo(FT_ULong charcode, Rect<PackingSizeType> charArea, std::set<FT_ULong> charsWithoutRect);

       private:
        void setFontInfo();
        void setKerningInfo(std::set<FT_ULong>::iterator charcodesBegin, std::set<FT_ULong>::iterator charcodesEnd);
        FT_Face face;
        std::string faceName;
        Info fontInfo;
        Common fontCommon;
        std::vector<CharInfo> charInfos;
        std::vector<KerningInfo> kerningInfos;
        FT_Pos maxYBearing;
        float scalingFactor;
        float padding;
    };
}
