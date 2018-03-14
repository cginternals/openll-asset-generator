#version 140
#extension GL_ARB_explicit_attrib_location : require

uniform mat4 modelView;
uniform mat4 projection;

layout (location = 0) in vec2 corner;
layout (location = 1) in vec2 texUV;

out vec4 color;
out vec2 v_uv;

void main()
{
    gl_Position = projection * modelView * vec4(corner * 2.0 - 1.0, -2.0, 1.0);
    color = vec4(corner, 0.0, 1.0);
	v_uv = texUV;
}