#include <float.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <llassetgen/FntWriter.h>
#include <llassetgen/Image.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H


namespace
{


float from_26_6_fixed_precision(const std::int32_t v)
{
    return float(v) / 64.0f;
}

float from_16_16_fixed_precision(const std::int32_t v)
{
    return float(v) / 65536.0f;
}


} // namespace


namespace llassetgen {
    FntWriter::FntWriter(FT_Face _face, std::string _faceName, unsigned int _fontSize, float _scalingFactor, float _padding) {
        faceName = _faceName;
        face = _face;
        fontInfo = Info();
        fontCommon = Common();
        charInfos = std::vector<CharInfo>();
        kerningInfos = std::vector<KerningInfo>();
        fontInfo.size = _fontSize;
        maxYBearing = 0;
        scalingFactor = _scalingFactor;
        padding = _padding;
    }

    void FntWriter::setFontInfo() {
        // collect fontInfo
        fontInfo.face = face->family_name + std::string(" ") + face->style_name;
        fontInfo.isBold = false;
        fontInfo.isItalic = false;
        if (face->style_flags == FT_STYLE_FLAG_ITALIC) {
            fontInfo.isItalic = true;
        }
        if (face->style_flags == FT_STYLE_FLAG_BOLD) {
            fontInfo.isBold = true;
        }
        int encoding = int(face->charmap->encoding);
        uint8_t* character = reinterpret_cast<uint8_t*>(&encoding);
        for (int i = 3; i >= 0; --i) {
            fontInfo.charset += char(character[i]);
        }

        fontInfo.useUnicode = (face->charmap->encoding == FT_ENCODING_UNICODE);

        //sizes are provided in 26.6 fixed-point format
        fontCommon.lineHeight = from_26_6_fixed_precision(face->size->metrics.height);
        // base is defined as "the distance from the ascent to the baseline"
        fontCommon.base = from_26_6_fixed_precision(face->size->metrics.ascender);
        fontCommon.descent = from_26_6_fixed_precision(face->size->metrics.descender);

        // haven't found any of the following information
        // irrelevant for distance fields --> ignore them
        /*
        fontInfo.stretchH = 1.0f;
        fontInfo.useSmoothing = false;
        fontInfo.supersamplingLevel = 1;
        */
        fontInfo.padding = { padding, padding, padding, padding };
        fontInfo.spacing = { 0.f, 0.f };
        /*
        fontInfo.outlineThickness = 0;
        */
    }

    /**
     * The set charsWithoutRect contains the code points of the (valid) characters that could not be rendered by freetype.
     * This is usually the case for spacing characters (e.g., space).
     * Returns a bool, stating wether the charArea was used (true) or ignored (false). This makes it easier to use
     * this function while iterating over charcodes and charAreas.
     */
    bool FntWriter::setCharInfo(FT_ULong charcode, Rect<PackingSizeType> charArea, std::set<FT_ULong> charsWithoutRect) {

        size_t charAreaPosX;
        size_t charAreaPosY;
        size_t charAreaWidth;
        size_t charAreaHeight;

        const bool charIsDepictable = charsWithoutRect.find(charcode) == charsWithoutRect.end();
        if (charIsDepictable) {
            charAreaPosX = charArea.position.x;
            charAreaPosY = charArea.position.y;
            charAreaWidth = charArea.size.x;
            charAreaHeight = charArea.size.y;
        } else {
            // for example, the space char is not depictable (thus, is not contained in the glyph texture), but
            // actually has a width in typesetting (called xAdvance).
            // The position in the texture atlas (at the pixel's color at that position) has no meaning, as the
            // width and height of that glyph in the texture is zero.
            charAreaPosX = 0;
            charAreaPosY = 0;
            charAreaWidth = 0;
            charAreaHeight = 0;
        }

        FT_UInt gindex = FT_Get_Char_Index(face, charcode);
        FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);

        // bearing is provided in 26.6 fixed - point format
        FT_Pos yBearing = from_26_6_fixed_precision(face->glyph->metrics.horiBearingY);
        maxYBearing = std::max(yBearing, maxYBearing);

        CharInfo charInfo;
        charInfo.id = charcode;
        charInfo.x = charAreaPosX;
        charInfo.y = charAreaPosY;
        charInfo.width = charAreaWidth;
        charInfo.height = charAreaHeight;
        charInfo.xAdvance = from_16_16_fixed_precision(face->glyph->linearHoriAdvance);
        charInfo.xOffset = from_26_6_fixed_precision(face->glyph->metrics.horiBearingX);
        charInfo.yOffset = yBearing;
        charInfo.page = 1;
        charInfo.chnl = 15;
        charInfos.push_back(charInfo);

        return charIsDepictable;
    }

    void FntWriter::setCharInfo(FT_ULong charcode, Rect<PackingSizeType> charArea) {
        FT_UInt gindex = FT_Get_Char_Index(face, charcode);
        FT_Load_Glyph(face, gindex, FT_LOAD_DEFAULT);

        //bearing is provided in 26.6 fixed - point format
        FT_Pos yBearing = from_26_6_fixed_precision(face->glyph->metrics.horiBearingY);
        maxYBearing = std::max(yBearing, maxYBearing);

        CharInfo charInfo;
        charInfo.id = charcode;
        charInfo.x = charArea.position.x;
        charInfo.y = charArea.position.y;
        charInfo.width = charArea.size.x;
        charInfo.height = charArea.size.y;
        charInfo.xAdvance = from_16_16_fixed_precision(face->glyph->linearHoriAdvance);
        charInfo.xOffset = from_26_6_fixed_precision(face->glyph->metrics.horiBearingX);
        charInfo.yOffset = yBearing;
        charInfo.page = 1;
        charInfo.chnl = 15;
        charInfos.push_back(charInfo);
    }

    void FntWriter::setKerningInfo(std::set<FT_ULong>::iterator charcodesBegin,
                                   std::set<FT_ULong>::iterator charcodesEnd) {
        for (std::set<FT_ULong>::iterator leftCharcode = charcodesBegin; leftCharcode != charcodesEnd; leftCharcode++) {
            for (std::set<FT_ULong>::iterator rightCharcode = charcodesBegin; rightCharcode != charcodesEnd;
                 rightCharcode++) {
                FT_UInt leftGindex = FT_Get_Char_Index(face, *leftCharcode);
                FT_UInt rightGindex = FT_Get_Char_Index(face, *rightCharcode);
                FT_Vector kerningVector;
                FT_Get_Kerning(face, leftGindex, rightGindex, FT_KERNING_UNFITTED, &kerningVector);
                if (kerningVector.x != 0) {
                    KerningInfo kerningInfo;
                    kerningInfo.firstId = *leftCharcode;
                    kerningInfo.secondId = *rightCharcode;
                    // kerning is provided in 26.6 fixed-point format
                    kerningInfo.kerning = from_26_6_fixed_precision(kerningVector.x);
                    kerningInfos.push_back(kerningInfo);
                }
            }
        }
    }

    void FntWriter::readFont(std::set<FT_ULong>::iterator charcodesBegin, std::set<FT_ULong>::iterator charcodesEnd) {
        setFontInfo();
        setKerningInfo(charcodesBegin, charcodesEnd);
    }

    void FntWriter::setAtlasProperties(Vec2<PackingSizeType> size) {
        // collect commonInfo
        fontCommon.scaleW = size.x;
        fontCommon.scaleH = size.y;
        fontCommon.pages = 1;
        fontCommon.isPacked = 0;
    }

    void FntWriter::saveFnt(std::string filepath) {
        // ascent is defined as "The distance from the baseline to the highest or upper grid coordinate used to
        // place an outline point." So set the maximum bearing over all glyphs as the overall ascent.
        fontCommon.ascent = maxYBearing;

        // open file
        std::ofstream fntFile;
        fntFile.open(filepath);

        // write in plain text format
        // TODO: XML and binary

        // write info block
        fntFile << "info "
                << "face=\"" << fontInfo.face << "\" "
                << "size=" << fontInfo.size * scalingFactor << " "
                << "bold=" << int(fontInfo.isBold) << " "
                << "italic=" << int(fontInfo.isItalic) << " "
                << "charset=\"" << fontInfo.charset << "\" "
                << "unicode="
                << int(fontInfo.useUnicode)
                << " "
                /*
                << "stretchH=" << fontInfo.stretchH << " "
                << "smooth=" << int(fontInfo.useSmoothing) << " "
                << "aa=" << fontInfo.supersamplingLevel << " "
                */
                << "padding=" << fontInfo.padding.up * scalingFactor << "," << fontInfo.padding.right * scalingFactor <<
                "," << fontInfo.padding.down * scalingFactor << "," << fontInfo.padding.left * scalingFactor << " "
                << "spacing=" << fontInfo.spacing.horiz * scalingFactor << "," << fontInfo.spacing.vert * scalingFactor << " "
                /*    
                << "outline=" << fontInfo.outlineThickness
                */
                << std::endl;

        // write common block
        fntFile << "common "
                << "lineHeight=" << float(fontCommon.lineHeight) * scalingFactor << " "
                << "base=" << float(fontCommon.base) * scalingFactor << " "
                << "ascent=" << float(fontCommon.ascent) * scalingFactor << " "
                << "descent=" << float(fontCommon.descent) * scalingFactor << " "
                << "scaleW=" << fontCommon.scaleW << " "
                << "scaleH=" << fontCommon.scaleH << " "
                << "pages=" << fontCommon.pages << " "
                << "packed=" << int(fontCommon.isPacked) << std::endl;

        // write page files
        for (int i = 0; i < fontCommon.pages; i++) {
            fntFile << "page "
                    << "id=" << i << " "
                    << "file=\"" << faceName << ".png" << "\"" << std::endl;
        }

        // write char count
        fntFile << "chars count=" << charInfos.size() << std::endl;

        // write info for each char
        for (auto charInfo : charInfos) {
            fntFile << "char "
                    << "id=" << charInfo.id << " "
                    << "x=" << charInfo.x << " "
                    << "y=" << charInfo.y << " "
                    << "width=" << charInfo.width << " "
                    << "height=" << charInfo.height << " "
                    << "xoffset=" << charInfo.xOffset * scalingFactor << " "
                    << "yoffset=" << (fontCommon.base - charInfo.yOffset) * scalingFactor << " "
                    << "xadvance=" << float(charInfo.xAdvance) * scalingFactor << " "
                    << "page=" << charInfo.page << " "
                    << "chnl=" << int(charInfo.chnl) << std::endl;
        }

        // write kerning count
        fntFile << "kernings count=" << kerningInfos.size() << std::endl;

        // write each kerning info
        for (auto kerningInfo : kerningInfos) {
            fntFile << "kerning "
                    << "first=" << kerningInfo.firstId << " "
                    << "second=" << kerningInfo.secondId << " "
                    << "amount=" << kerningInfo.kerning * scalingFactor << std::endl;
        }

        // close file
        fntFile.close();
    }
}
