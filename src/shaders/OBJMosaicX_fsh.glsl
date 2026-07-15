#version 460

layout (binding = 0) uniform usampler2D SpriteLayer;
layout (binding = 1) uniform usampler2D OBJMosaicIndex;
layout (binding = 2) uniform usampler2D MosaicTable;

layout (location = 0) out uint FinalColor;

layout (std140, binding = 0) uniform OBJMosaicUniform
{
    uint MosaicSizeX;
    uint pad0, pad1, pad2;
};

void main()
{
    ivec2 position = ivec2(gl_FragCoord.xy);
    uint index = texelFetch(OBJMosaicIndex, position, 0).r;
    if ((index & 0x100U) == 0U || MosaicSizeX == 0U)
    {
        FinalColor = texelFetch(SpriteLayer, position, 0).r;
        return;
    }

    uint spriteID = index & 0xFFU;
    int sourceX = position.x;
    int groupStart = int(texelFetch(MosaicTable, ivec2(position.x, int(MosaicSizeX)), 0).r);

    for (int offset = 1; offset < 16; offset++)
    {
        int candidateX = position.x - offset;
        if (candidateX < groupStart)
            break;

        uint candidateIndex = texelFetch(OBJMosaicIndex, ivec2(candidateX, position.y), 0).r;
        if ((candidateIndex & 0xFFU) != spriteID)
            break;

        sourceX = candidateX;
    }

    FinalColor = texelFetch(SpriteLayer, ivec2(sourceX, position.y), 0).r;
}
