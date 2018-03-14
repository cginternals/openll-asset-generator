#version 140
#extension GL_ARB_explicit_attrib_location : require

const uint SuperSamplingNone     = 0u;
const uint SuperSampling1x3      = 1u;
const uint SuperSampling2x4      = 2u;
const uint SuperSampling2x2RGSS  = 3u;
const uint SuperSamplingQuincunx = 4u;
const uint SuperSampling8Rooks   = 5u;
const uint SuperSampling3x3      = 6u;
const uint SuperSampling4x4      = 7u;

uniform vec4 fontColor;
uniform sampler2D glyphs;
uniform uint superSampling;

in vec4 color;
in vec2 v_uv;

layout (location = 0) out vec4 fragColor;

const int channel = 0;

/**
 * Supersampling by OpenLL
 */

float aastep(float t, float value)
{
    float afwidth = fwidth(value);
    return smoothstep(t - afwidth, t + afwidth, value);
}

float tex(float t, vec2 uv)
{
    return aastep(0.5, texture(glyphs, uv)[channel]);
}

float aastep1x3(float t, vec2 uv)
{
    float y = dFdy(uv.y) * 1.0 / 3.0;

    float v = tex(t, uv + vec2(0,-y))
            + tex(t, uv + vec2(0, 0))
            + tex(t, uv + vec2(0,+y));

    return v / 3.0;
}

// rotated grid
float aastep2x2RGSS(float t, vec2 uv)
{
    float x1 = dFdx(uv.x) * 1.0 / 8.0;
    float y1 = dFdy(uv.y) * 1.0 / 8.0;
    float x2 = dFdx(uv.x) * 3.0 / 8.0;
    float y2 = dFdy(uv.y) * 3.0 / 8.0;

    float v = tex(t, uv + vec2(-x2,+y1))
            + tex(t, uv + vec2(-x1,-y2))
            + tex(t, uv + vec2(+x2,-y1))
            + tex(t, uv + vec2(+x1,+y2));

    return v / 4.0;
}

float aastepQuincunx(float t, vec2 uv)
{
    float x = dFdx(uv.x) / 2.0;
    float y = dFdy(uv.y) / 2.0;

    float v = tex(t, uv) * 4
            + tex(t, uv + vec2(-x,+y))
            + tex(t, uv + vec2(-x,-y))
            + tex(t, uv + vec2(+x,+y))
            + tex(t, uv + vec2(+x,-y));

    return v / 8.0;
}

float aastep8Rooks(float t, vec2 uv)
{
    float x1 = dFdx(uv.x) * 1.0 / 16.0;
    float x2 = dFdx(uv.x) * 3.0 / 16.0;
    float x3 = dFdx(uv.x) * 5.0 / 16.0;
    float x4 = dFdx(uv.x) * 7.0 / 16.0;
    float y1 = dFdy(uv.y) * 1.0 / 16.0;
    float y2 = dFdy(uv.y) * 3.0 / 16.0;
    float y3 = dFdy(uv.y) * 5.0 / 16.0;
    float y4 = dFdy(uv.y) * 7.0 / 16.0;

    float v = tex(t, uv + vec2(-x4,+y2))
            + tex(t, uv + vec2(-x3,-y1))
            + tex(t, uv + vec2(-x2,+y3))
            + tex(t, uv + vec2(-x1,-y4))
            + tex(t, uv + vec2(+x1,+y4))
            + tex(t, uv + vec2(+x2,-y3))
            + tex(t, uv + vec2(+x3,+y1))
            + tex(t, uv + vec2(+x4,-y2));

    return v / 8.0;
}

float aastep2x4(float t, vec2 uv)
{
    float x1 = dFdx(uv.x) * 1.0 / 4.0;
    float y1 = dFdy(uv.y) * 1.0 / 8.0;
    float y2 = dFdy(uv.y) * 3.0 / 8.0;

    float v = tex(t, uv + vec2(-x1,-y2))
            + tex(t, uv + vec2(-x1,-y1))
            + tex(t, uv + vec2(-x1,+y1))
            + tex(t, uv + vec2(-x1,+y2))

            + tex(t, uv + vec2(+x1,-y2))
            + tex(t, uv + vec2(+x1,-y1))
            + tex(t, uv + vec2(+x1,+y1))
            + tex(t, uv + vec2(+x1,+y2));

    return v / 8.0;
}

float aastep3x3(float t, vec2 uv)
{
    float x = dFdx(uv.x) * 1.0 / 3.0;
    float y = dFdy(uv.y) * 1.0 / 3.0;

    float v = tex(t, uv + vec2(-x,-y))
            + tex(t, uv + vec2(-x, 0))
            + tex(t, uv + vec2(-x,+y))

            + tex(t, uv + vec2( 0,-y))
            + tex(t, uv + vec2( 0, 0))
            + tex(t, uv + vec2( 0,+y))

            + tex(t, uv + vec2(+x,-y))
            + tex(t, uv + vec2(+x, 0))
            + tex(t, uv + vec2(+x,+y));

    return v / 9.0;
}

float aastep4x4(float t, vec2 uv)
{
    float x1 = dFdx(uv.x) * 1.0 / 8.0;
    float y1 = dFdy(uv.y) * 1.0 / 8.0;
    float x2 = dFdx(uv.x) * 3.0 / 8.0;
    float y2 = dFdy(uv.y) * 3.0 / 8.0;

    float v = tex(t, uv + vec2(-x2,-y2))
            + tex(t, uv + vec2(-x2,-y1))
            + tex(t, uv + vec2(-x2,+y1))
            + tex(t, uv + vec2(-x2,+y2))

            + tex(t, uv + vec2(-x1,-y2))
            + tex(t, uv + vec2(-x1,-y1))
            + tex(t, uv + vec2(-x1,+y1))
            + tex(t, uv + vec2(-x1,+y2))

            + tex(t, uv + vec2(+x1,-y2))
            + tex(t, uv + vec2(+x1,-y1))
            + tex(t, uv + vec2(+x1,+y1))
            + tex(t, uv + vec2(+x1,+y2))

            + tex(t, uv + vec2(+x2,-y2))
            + tex(t, uv + vec2(+x2,-y1))
            + tex(t, uv + vec2(+x2,+y1))
            + tex(t, uv + vec2(+x2,+y2));

    return v / 16.0;
}



void main()
{    
    // requires blend: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float s = texture(glyphs, v_uv).r;
    if(s < 0.3)
        discard;

    //vec4 fc = color; // if we want vertex-based color
    vec4 fc = fontColor; // if we want uniform font color

    float a;
    switch (superSampling)
    {
    case SuperSamplingNone:     a =            tex(0.5, v_uv); break;
    case SuperSampling1x3:      a =      aastep1x3(0.5, v_uv); break;
    case SuperSampling2x4:      a =      aastep2x4(0.5, v_uv); break;
    case SuperSampling2x2RGSS:  a =  aastep2x2RGSS(0.5, v_uv); break;
    case SuperSamplingQuincunx: a = aastepQuincunx(0.5, v_uv); break;
    case SuperSampling8Rooks:   a =   aastep8Rooks(0.5, v_uv); break;
    case SuperSampling3x3:      a =      aastep3x3(0.5, v_uv); break;
    case SuperSampling4x4:      a =      aastep4x4(0.5, v_uv); break;
    }

    fragColor = vec4(fc.rgb, fc.a * a);
}