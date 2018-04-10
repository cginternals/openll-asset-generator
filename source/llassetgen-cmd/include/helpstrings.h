#pragma once

std::string
    atlasHelp{"Create a font atlas, optionally applying a distance transform"},
    distfieldHelp{"Apply a distance transform algorithm to the atlas. If none is chosen, no distance transform will be applied"},
    packingHelp{"Use a different packing algorithm. 'maxrects' is more space-efficient, 'shelf' is faster"},
    glyphHelp{"Add the specified glyphs to the atlas"},
    charcodeHelp{"Add glyphs to the atlas by specifying their character codes, separated by spaces"},
    fontnameHelp{"Use the font with the specified name"},
    fontpathHelp{"Use the font file at the specified path"},
    paddingHelp{"Add padding to each glyph"},
    fontsizeHelp{"Specify the font size in pixels"},
    dynamicrangeHelp{""},  // TODO
    asciiHelp{"Add all printable ASCII glyphs"},
    aOutfileHelp{"Output the font atlas to the specified path"},

    dfHelp{"Apply a distance transform to an image"},
    algorithmHelp{"Apply a different distance transform algorithm to the atlas"},
    imageHelp{"Apply the distance transform to the image at this path"},
    dOutfileHelp{"Output the distance field to the specified path"};
