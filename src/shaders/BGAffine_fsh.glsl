#version 460

#cmakedefine Affine
#cmakedefine ExtendedBitmap8bpp
#cmakedefine ExtendedBitmapDirectColor
#cmakedefine ExtendedMixed
#cmakedefine Mosaic

layout (binding = 0) uniform usamplerBuffer BGVRAM8;
layout (binding = 1) uniform usamplerBuffer BGVRAM16;
layout (binding = 2) uniform usampler2D MosaicTable;

layout (location = 0) out uint FinalColor;

layout (std140, binding = 0) uniform BGUniform
{
    int TilesetAddr, TilemapAddr, BGVRAMMask, YShift;
    int XMask, YMask, OfxMask, OfyMask;
    uint MetaMask, ExtPalMask, RenderScale, MosaicLevel;
    ivec4 PerLineData[192]; // rotX, rotY, rotA, rotC
};

void main()
{
    ivec2 position = ivec2(gl_FragCoord.xy);
    ivec2 nativePosition = position;
    int scale = max(int(RenderScale), 1);
    uint mosaicLevel = MosaicLevel & 0xFFU;
    int batchEndLine = int(MosaicLevel >> 8);
    int subY = 0;

    if (RenderScale == 2U)
    {
        nativePosition = position >> 1;
        subY = position.y & 1;
    }
    else if (RenderScale == 4U)
    {
        nativePosition = position >> 2;
        subY = position.y & 3;
    }

#ifdef Mosaic
    nativePosition.x = int(texelFetch(MosaicTable, ivec2(nativePosition.x, int(mosaicLevel)), 0).x);
    position.x = nativePosition.x * scale;
#endif

    ivec4 perLineData = PerLineData[nativePosition.y];
    ivec2 lineBase = perLineData.xy;

#ifndef Mosaic
    if (scale > 1 && nativePosition.y < 191 && nativePosition.y + 1 < batchEndLine)
    {
        ivec2 nextLineBase = PerLineData[nativePosition.y + 1].xy;
        if (RenderScale == 2U)
            lineBase += ((nextLineBase - lineBase) * subY) / 2;
        else if (RenderScale == 4U)
            lineBase += ((nextLineBase - lineBase) * subY) / 4;
    }
#endif

    ivec2 xStep = position.x * perLineData.zw;
    if (RenderScale == 2U)
        xStep /= 2;
    else if (RenderScale == 4U)
        xStep /= 4;

    ivec2 rot = lineBase + xStep;

#if defined(ExtendedMixed) || defined(Affine)
    if (((rot.x | rot.y) & OfxMask) == 0)
    {
#ifdef Affine
        int tileoffset = (((rot.y & XMask) >> 11) << (YShift & 0x1F)) + ((rot.x & XMask) >> 11);

        uint tile = texelFetch(BGVRAM8, (TilemapAddr + tileoffset) & BGVRAMMask).r;

        int tilexoff = (rot.x >> 8) & 0x7;
        int tileyoff = (rot.y >> 8) & 0x7;

        int coloroffset = (int(tile & 0x03FF) << 6) + (tileyoff << 3) + tilexoff;

        uint color = texelFetch(BGVRAM8, (TilesetAddr + coloroffset) & BGVRAMMask).r;

        FinalColor = color == 0 ? 0 : (color | MetaMask);
#endif
#ifdef ExtendedMixed
        int tileoffset = ((((rot.y & XMask) >> 11) << (YShift & 0x1F)) + ((rot.x & XMask) >> 11)) << 1;

        uint tile = texelFetch(BGVRAM16, ((TilemapAddr + tileoffset) & BGVRAMMask) >> 1).r;

        int tilexoff = (rot.x >> 8) & 0x7;
        int tileyoff = (rot.y >> 8) & 0x7;

        if ((tile & 0x0400U) != 0) tilexoff = 7-tilexoff;
        if ((tile & 0x0800U) != 0) tileyoff = 7-tileyoff;

        int coloroffset = (int(tile & 0x03FF) << 6) + (tileyoff << 3) + tilexoff;

        uint color = texelFetch(BGVRAM8, (TilesetAddr + coloroffset) & BGVRAMMask).r;

        FinalColor = color == 0 ? 0 : (color | MetaMask) + (((tile >> 12) << 8) & ExtPalMask);
#endif
    }
#else
    if ((rot.x & OfxMask) == 0 && (rot.y & OfyMask) == 0)
    {
#ifdef ExtendedBitmap8bpp
        int addrOffset = (((rot.y & YMask) >> 8) << YShift) + ((rot.x & XMask) >> 8);

        uint color = texelFetch(BGVRAM8, (TilemapAddr + addrOffset) & BGVRAMMask).r;
        FinalColor = color == 0 ? 0 : color | MetaMask;
#endif
#ifdef ExtendedBitmapDirectColor
        int addrOffset = ((((rot.y & YMask) >> 8) << YShift) + ((rot.x & XMask) >> 8)) << 1;

        uint color = texelFetch(BGVRAM16, ((TilemapAddr + addrOffset) & BGVRAMMask) >> 1).r;

        if ((color & 0x8000) == 0)
        {
            FinalColor = 0;
        }
        else
        {
            FinalColor = MetaMask
                | ((color & 0x1FU) << 1)
                | ((color & 0x3E0U) << 4)
                | ((color & 0x7C00U) << 7);
        }
#endif
    }
#endif
    else
    {
        FinalColor = 0;
    }
}
