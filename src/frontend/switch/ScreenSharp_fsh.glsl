#version 460

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 coolTransparency;

layout (binding = 0) uniform sampler2D inTexture;

void main()
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 texel = inUV * texSize - 0.5;
    vec2 texelBase = floor(texel);
    vec2 texelFrac = fract(texel);
    vec2 texelWidth = max(fwidth(texel), vec2(0.0001));
    vec2 sharpFrac = clamp((texelFrac - 0.5) / texelWidth + 0.5, 0.0, 1.0);

    outColor = texture(inTexture, (texelBase + sharpFrac + 0.5) / texSize) * inColor;
    outColor.a *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
}
