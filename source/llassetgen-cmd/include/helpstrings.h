#pragma once

std::string
    appHelp{"OpenLL Font Asset Generator\nRun 'llassetgen [SUBCOMMAND] --help' for more details\n"},
    atlasHelp{"Create a font atlas, optionally applying a distance transform"},
    distfieldHelp{
        "Apply a distance transform algorithm to the atlas. If none is chosen, no distance transform will be applied"},
    packingHelp{"Use a different packing algorithm. 'maxrects' is more space-efficient, 'shelf' is faster"},
    glyphHelp{"Add the specified glyphs to the atlas"},
    charcodeHelp{"Add glyphs to the atlas by specifying their character codes, separated by spaces"},
    fontnameHelp{"Use the font with the specified name"}, fontpathHelp{"Use the font file at the specified path"},
    paddingHelp{"Add padding to each glyph"}, fontsizeHelp{"Specify the font size in pixels"},
    dynamicrangeHelp{
        "Takes two values, BLACK and WHITE. A lower black value will make the distance fields wider; a lower white "
        "value will make the distance fields brighter. In most cases, the black value should be lower than the white "
        "value. However, swapping the black and white value will invert the colors of the atlas"},
    asciiHelp{"Add all printable ASCII glyphs"}, aOutfileHelp{"Output the font atlas to the specified path"},
    configHelp{
        "Read options from a configuration file. Options passed as arguments will override the configuration file. You "
        "can find an example file in the 'config' directory"},
    fntHelp{"Generate a font file in the FNT format"}, downsamplingRatioHelp{"Downsample the atlas by this factor."},
    downsamplingHelp{"Use a different downsampling algorithm"},

    dfHelp{"Apply a distance transform to an image"},
    algorithmHelp{"Apply a different distance transform algorithm to the atlas"},
    imageHelp{"Apply the distance transform to the image at this path"},
    dOutfileHelp{"Output the distance field to the specified path"};
