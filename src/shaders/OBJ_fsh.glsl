#version 460

layout (binding = 0) uniform usamplerBuffer OBJVRAM8;
layout (binding = 1) uniform usamplerBuffer OBJVRAM16;
layout (binding = 2) uniform usampler2D MosaicTable;

#cmakedefine OBJ4bpp
#cmakedefine OBJ8bpp
#cmakedefine OBJBitmap
#cmakedefine OBJWindow

layout (location = 0) in vec2 InInSpritePosition;
layout (location = 1) flat in uvec4 InAddrInfo;
layout (location = 2) flat in uvec2 InSize;

layout (location = 0) out uint FinalColor;

layout (std140, binding = 0) uniform OBJUniform
{
    int OBJVRAMMask;
    vec4 AffineTransforms[32];
};

const uint OBJMosaicCoordMask = 0x1FFU;
const uint OBJMosaicXShift = 0U;
const uint OBJMosaicYShift = 9U;
const uint OBJMosaicSizeXShift = 18U;
const uint OBJMosaicSizeYShift = 22U;
const uint OBJMosaicHFlip = 1U << 26U;
const uint OBJMosaicVFlip = 1U << 27U;
const uint OBJMosaicEnable = 1U << 28U;

int DecodeMosaicCoord(uint value)
{
    int coord = int(value & OBJMosaicCoordMask);
    if (coord >= 0x100)
        coord -= 0x200;
    return coord;
}

void main()
{
    ivec2 inSpritePosition = ivec2(InInSpritePosition);
    if ((InAddrInfo.w & OBJMosaicEnable) != 0U)
    {
        uint mosaicLevelX = (InAddrInfo.w >> OBJMosaicSizeXShift) & 0xFU;
        if (mosaicLevelX != 0U)
        {
            int spriteX = DecodeMosaicCoord(InAddrInfo.w >> OBJMosaicXShift);
            int screenX = int(gl_FragCoord.x);
            int mosaicX = int(texelFetch(MosaicTable, ivec2(screenX, int(mosaicLevelX)), 0).x);
            int sourceX = mosaicX - spriteX;
            if ((InAddrInfo.w & OBJMosaicHFlip) != 0U)
                sourceX = int(InSize.x >> 8) - 1 - sourceX;

            inSpritePosition.x = sourceX * 256;
        }

        uint mosaicLevelY = (InAddrInfo.w >> OBJMosaicSizeYShift) & 0xFU;
        if (mosaicLevelY != 0U)
        {
            int spriteY = DecodeMosaicCoord(InAddrInfo.w >> OBJMosaicYShift);
            int screenY = int(gl_FragCoord.y);
            int mosaicY = int(texelFetch(MosaicTable, ivec2(screenY, int(mosaicLevelY)), 0).x);
            int sourceY = mosaicY - spriteY;
            if ((InAddrInfo.w & OBJMosaicVFlip) != 0U)
                sourceY = int(InSize.y >> 8) - 1 - sourceY;

            inSpritePosition.y = sourceY * 256;
        }
    }

    if (uint(inSpritePosition.x) >= InSize.x
        || uint(inSpritePosition.y) >= InSize.y)
        discard;

    int basepixeladdr = int(InAddrInfo.x);
    int yshift = int(InAddrInfo.y);
    uint meta = InAddrInfo.z;

    ivec2 position = inSpritePosition >> 11;
    ivec2 tile = (inSpritePosition >> 8) & ivec2(7);

#ifdef OBJ4bpp
    int pixeladdr = (tile.y << 2) + (position.x << 5) + (position.y << yshift) + (tile.x >> 1);

    uint pixel = texelFetch(OBJVRAM8, (basepixeladdr + pixeladdr) & OBJVRAMMask).r;
    if ((tile.x & 1) == 0)
        pixel &= 0xFU;
    else
        pixel >>= 4U;

#ifdef OBJWindow
    FinalColor = uint(pixel != 0U);
#else
    if (pixel == 0)
        discard;
    FinalColor = meta | pixel;
#endif
#endif
#ifdef OBJ8bpp
    int pixeladdr = (tile.y << 3) + (position.x << 6) + (position.y << yshift) + tile.x;

    uint pixel = texelFetch(OBJVRAM8, (basepixeladdr + pixeladdr) & OBJVRAMMask).r;

#ifdef OBJWindow
    FinalColor = uint(pixel != 0U);
#else
    if (pixel == 0)
        discard;
    FinalColor = meta | pixel;
#endif
#endif
#ifdef OBJBitmap
    int pixeladdr = ((inSpritePosition.y >> 8) << yshift) + ((inSpritePosition.x >> 8) << 1);

    uint pixel = texelFetch(OBJVRAM16, ((basepixeladdr + pixeladdr) & OBJVRAMMask) >> 1).r;

    if ((pixel & 0x8000U) == 0)
        discard;

    FinalColor = meta
        | ((pixel & 0x1FU) << 1)
        | ((pixel & 0x3E0U) << 4)
        | ((pixel & 0x7C00U) << 7);
#endif
}
