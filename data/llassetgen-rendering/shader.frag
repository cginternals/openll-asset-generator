#version 140
#extension GL_ARB_explicit_attrib_location : require

layout (location = 0) out vec4 fragColor;

in vec4 color;
in vec2 v_uv;
uniform sampler2D glyphs;


const int channel = 0;

float aastep(float t, float value)
{
    float afwidth = fwidth(value);
    return smoothstep(t - afwidth, t + afwidth, value);
}

float tex(float t, vec2 uv)
{
    return aastep(0.5, texture(glyphs, uv)[channel]);
}


void main()
{	
    float s = texture(glyphs, v_uv).r;
    if(s < 0.3)
        discard;

    vec4 fc = color; //vec4(1.0, 0.0, 0.0, 1.0);

    float a = tex(0.5, v_uv);

    fragColor = vec4(fc.rgb, fc.a * a);
}