#if GL_ES
precision mediump int;
precision mediump float;
precision mediump sampler2DShadow;
#endif

in vec2 _texCoords;
out vec4 frag_color;
uniform sampler2D GM_framebuffer;

uniform int GM_HDR;
uniform int GM_ToneMapping;
vec4 calculateWithToneMapping(vec4 c)
{
    const int ReinhardToneMapping = 0;

    if (GM_HDR > 0)
    {
        if (GM_ToneMapping == ReinhardToneMapping)
        {
            vec3 color = c.rgb;
            return vec4(color / (color + vec3(1,1,1)), 1);
        }
    }
    return c;
}

struct GMScreenInfo
{
    int GM_Multisampling;
    int GM_ScreenWidth;
    int GM_ScreenHeight;
};
uniform GMScreenInfo GM_ScreenInfo;
uniform float GM_KernelDeltaX;
uniform float GM_KernelDeltaY;
uniform vec3 GM_BlendFactor;

vec3 kernel(float kernels[9], sampler2D t, vec2 uv)
{
    float X = GM_KernelDeltaX / float(GM_ScreenInfo.GM_ScreenWidth);
    float Y = GM_KernelDeltaY / float(GM_ScreenInfo.GM_ScreenHeight);

    vec2 offsets[9] = vec2[](
        vec2(-X, -Y),
        vec2( 0.0f, -Y),
        vec2( X, -Y),
        vec2(-X, 0.0f),  
        vec2( 0.0f, 0.0f),  
        vec2( X, 0.0f),
        vec2(-X, Y),
        vec2( 0.0f, Y),
        vec2( X, Y)
    );
    vec3 sampler[9];
    for(int i = 0; i < 9; i++)
    {
        sampler[i] = vec3(texture(t, uv + offsets[i]));
    }
    vec3 color = vec3(0);
    for(int i = 0; i < 9; i++)
        color += sampler[i] * kernels[i];
    return color;
}

vec3 GM_InversionFilter(sampler2D t, vec2 uv)
{
    vec3 color = texture(t, uv).rgb;
    return vec3(1.f - color.r, 1.f - color.g, 1.f - color.b);
}

vec3 GM_SharpenFilter(sampler2D t, vec2 uv)
{
    float kernels[9] = float[](
        -1.f, -1.f, -1.f,
        -1.f,  9.f, -1.f,
        -1.f, -1.f, -1.f
    );
    return kernel(kernels, t, uv);
}

vec3 GM_BlurFilter(sampler2D t, vec2 uv)
{
    float kernels[9] = float[](
        1.f / 16.f, 2.f / 16.f, 1.f / 16.f,
        2.f / 16.f, 4.f / 16.f, 2.f / 16.f,
        1.f / 16.f, 2.f / 16.f, 1.f / 16.f 
    );
    return kernel(kernels, t, uv);
}

vec3 GM_GrayscaleFilter(sampler2D t, vec2 uv)
{
    vec3 fragColor = texture(t, uv).rgb;
    float average = 0.2126f * fragColor.r + 0.7152f * fragColor.g + 0.0722f * fragColor.b;
    return vec3(average, average, average);
}

vec3 GM_EdgeDetectFilter(sampler2D t, vec2 uv)
{
    float kernels[9] = float[](
        1.f, 1.f, 1.f,
        1.f, -8.f, 1.f,
        1.f, 1.f, 1.f
    );
    return kernel(kernels, t, uv);
}

vec3 GM_BlendFilter(sampler2D t, vec2 uv)
{
    return texture(t, uv).rgb * GM_BlendFactor;
}

vec3 GM_DefaultFilter(sampler2D t, vec2 uv)
{
    return texture(t, uv).rgb;
}

// 子程序选择，对应gmglshaderprogram.cpp@toTechniqueEntranceId
uniform int GM_Filter;
const int GM_FilterType_DefaultFilter = 0;
const int GM_FilterType_InversionFilter = 1;
const int GM_FilterType_SharpenFilter = 2;
const int GM_FilterType_BlurFilter = 3;
const int GM_FilterType_GrayscaleFilter = 4;
const int GM_FilterType_EdgeDetectFilter = 5;
const int GM_FilterType_BlendFilter = 6;

vec3 GM_InvokeFilter(sampler2D t, vec2 texcoords)
{
    // 与GMModelType的顺序一致
    switch (GM_Filter)
    {
        case GM_FilterType_DefaultFilter:
            return GM_DefaultFilter(t, texcoords);
        case GM_FilterType_InversionFilter:
            return GM_InversionFilter(t, texcoords);
        case GM_FilterType_SharpenFilter:
            return GM_SharpenFilter(t, texcoords);
        case GM_FilterType_BlurFilter:
            return GM_BlurFilter(t, texcoords);
        case GM_FilterType_GrayscaleFilter:
            return GM_GrayscaleFilter(t, texcoords);
        case GM_FilterType_EdgeDetectFilter:
            return GM_EdgeDetectFilter(t, texcoords);
        case GM_FilterType_BlendFilter:
            return GM_BlendFilter(t, texcoords);
    }
    return vec3(0.f, 0.f, 0.f);
}

void main()
{
    frag_color = vec4(max(GM_InvokeFilter(GM_framebuffer, _texCoords), vec3(0.f, 0.f, 0.f)), 1.f);
    frag_color = calculateWithToneMapping(frag_color);
}
