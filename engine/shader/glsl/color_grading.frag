#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp subpassInput in_color;

layout(set = 0, binding = 1) uniform sampler2D color_grading_lut_texture_sampler;

layout(location = 0) out highp vec4 out_color;

void main()
{
    highp ivec2 lut_tex_size = textureSize(color_grading_lut_texture_sampler, 0);

    highp vec4 color       = subpassLoad(in_color).rgba;

    // 采样，其中蓝色索引要化成整数
    highp float red_index  =  color.r * 15.0;
    highp float blue_index  =  floor(color.b * 15.0);
    highp float green_index  =  color.g * 15.0;
    highp float u = (red_index + blue_index * 16.0) / 255.0;
    highp float v  = green_index / 15.0;
    highp vec2 uv = vec2(u, v);
    out_color = texture(color_grading_lut_texture_sampler, uv);

    // 仅针对蓝色进行插值操作
    if(blue_index != color.b * 15.0)
    {
        highp vec4 sample_color_left = out_color;

        blue_index = ceil(color.b * 15.0);
        u = (red_index + blue_index * 16.0) / 255.0;
        uv = vec2(u, v);
        highp vec4 sample_color_right = texture(color_grading_lut_texture_sampler, uv);

        out_color = (ceil(color.b * 15.0) - color.b * 15.0) * sample_color_left + (color.b * 15.0 - floor(color.b * 15.0)) * sample_color_right;
    }
}
