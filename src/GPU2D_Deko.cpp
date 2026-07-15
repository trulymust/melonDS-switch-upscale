#include "GPU2D_Deko.h"
#include "GPU.h"

#include <arm_neon.h>
#include <assert.h>

#include "frontend/switch/Gfx.h"

#include "GPU3D_Deko.h"

#include <assert.h>

using Gfx::EmuCmdBuf;
using Gfx::EmuQueue;

namespace GPU2D
{

DekoRenderer::DekoRenderer() :
    CmdMem(*Gfx::DataHeap, 1024*1024*2)
{
    memset(FramebufferReady, 0, sizeof(dk::Fence)*2);
    memset(FramebufferPresented, 0, sizeof(dk::Fence)*2);

    dk::ImageLayout finalFbLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(FinalFramebufferWidth, FinalFramebufferHeight)
        .setFlags(DkImageFlags_UsageRender)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .initialize(finalFbLayout);
    FinalFramebufferMemory = Gfx::TextureHeap->Alloc(finalFbLayout.getSize() * 4, finalFbLayout.getAlignment());
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 2; i++)
        {
            FinalFramebuffers[j][i].initialize(finalFbLayout,
                Gfx::TextureHeap->MemBlock,
                FinalFramebufferMemory.Offset + finalFbLayout.getSize() * (i + j * 2));
        }
    }

    dk::ImageLayout displayCaptureColorLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(NativeWidth, NativeHeight)
        .setFlags(DkImageFlags_UsageRender)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .initialize(displayCaptureColorLayout);
    DisplayCaptureColorTargetMemory = Gfx::TextureHeap->Alloc(displayCaptureColorLayout.getSize(), displayCaptureColorLayout.getAlignment());
    DisplayCaptureColorTarget.initialize(displayCaptureColorLayout,
        Gfx::TextureHeap->MemBlock,
        DisplayCaptureColorTargetMemory.Offset);

    dk::ImageLayout intermedFbLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(256, 192)
        .setFlags(DkImageFlags_UsageRender|DkImageFlags_UsageLoadStore|DkImageFlags_Usage2DEngine)
        .setFormat(DkImageFormat_R32_Uint)
        .initialize(intermedFbLayout);
    IntermedFramebufferMemory = Gfx::TextureHeap->Alloc(intermedFbLayout.getSize() * fb_Count * 2, intermedFbLayout.getAlignment());
    for (int i = 0; i < fb_Count * 2; i++)
    {
        IntermedFramebuffers[i].initialize(intermedFbLayout,
            Gfx::TextureHeap->MemBlock,
            IntermedFramebufferMemory.Offset + intermedFbLayout.getSize() * i);
    }

    dk::ImageLayout hiResIntermedFbLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(FinalFramebufferWidth, FinalFramebufferHeight)
        .setFlags(DkImageFlags_UsageRender|DkImageFlags_UsageLoadStore|DkImageFlags_Usage2DEngine)
        .setFormat(DkImageFormat_R32_Uint)
        .initialize(hiResIntermedFbLayout);
    IntermedFramebufferHiResMemory = Gfx::TextureHeap->Alloc(hiResIntermedFbLayout.getSize() * fb_Count * 2, hiResIntermedFbLayout.getAlignment());
    for (int i = 0; i < fb_Count * 2; i++)
    {
        IntermedFramebuffersHiRes[i].initialize(hiResIntermedFbLayout,
            Gfx::TextureHeap->MemBlock,
            IntermedFramebufferHiResMemory.Offset + hiResIntermedFbLayout.getSize() * i);
    }

    _3DFramebufferMemory = Gfx::TextureHeap->Alloc(intermedFbLayout.getSize(), intermedFbLayout.getAlignment());
    _3DFramebuffer.initialize(intermedFbLayout, Gfx::TextureHeap->MemBlock, _3DFramebufferMemory.Offset);

    dk::ImageLayout hiRes3DFramebufferLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(FinalFramebufferWidth, FinalFramebufferHeight)
        .setFlags(DkImageFlags_UsageRender|DkImageFlags_UsageLoadStore|DkImageFlags_Usage2DEngine)
        .setFormat(DkImageFormat_R32_Uint)
        .initialize(hiRes3DFramebufferLayout);
    _3DFramebufferHiResMemory = Gfx::TextureHeap->Alloc(hiRes3DFramebufferLayout.getSize(), hiRes3DFramebufferLayout.getAlignment());
    _3DFramebufferHiRes.initialize(hiRes3DFramebufferLayout, Gfx::TextureHeap->MemBlock, _3DFramebufferHiResMemory.Offset);

    dk::ImageLayout objWindowLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(256, 192)
        .setFlags(DkImageFlags_UsageRender)
        .setFormat(DkImageFormat_R8_Uint)
        .initialize(objWindowLayout);
    OBJWindowMemory = Gfx::TextureHeap->Alloc(objWindowLayout.getSize()*2, objWindowLayout.getAlignment());
    for (int i = 0; i < 2; i++)
    {
        OBJWindow[i].initialize(objWindowLayout, Gfx::TextureHeap->MemBlock, OBJWindowMemory.Offset + objWindowLayout.getSize()*i);
    }

    dk::ImageLayout objDepthLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(256, 192)
        .setFlags(DkImageFlags_UsageRender)
        .setFormat(DkImageFormat_Z16)
        .initialize(objDepthLayout);
    OBJDepthMemory = Gfx::TextureHeap->Alloc(objDepthLayout.getSize(), objDepthLayout.getAlignment());
    OBJDepth.initialize(objDepthLayout, Gfx::TextureHeap->MemBlock, OBJDepthMemory.Offset);

    dk::ImageLayout objDepthHiResLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setDimensions(FinalFramebufferWidth, FinalFramebufferHeight)
        .setFlags(DkImageFlags_UsageRender)
        .setFormat(DkImageFormat_Z16)
        .initialize(objDepthHiResLayout);
    OBJDepthHiResMemory = Gfx::TextureHeap->Alloc(objDepthHiResLayout.getSize(), objDepthHiResLayout.getAlignment());
    OBJDepthHiRes.initialize(objDepthHiResLayout, Gfx::TextureHeap->MemBlock, OBJDepthHiResMemory.Offset);

    const u32 TextureVRAMSizes[textureVRAM_Count] = {512*1024, 128*1024, 256*1024, 128*1024};
    for (int i = 0; i < textureVRAM_Count; i++)
    {
        dk::ImageLayout vramTextureLayout8;
        dk::ImageLayoutMaker{Gfx::Device}
            .setType(DkImageType_Buffer)
            .setDimensions(TextureVRAMSizes[i])
            .setFormat(DkImageFormat_R8_Uint)
            .initialize(vramTextureLayout8);
        dk::ImageLayout vramTextureLayout16;
        dk::ImageLayoutMaker{Gfx::Device}
            .setType(DkImageType_Buffer)
            .setDimensions(TextureVRAMSizes[i] / 2)
            .setFormat(DkImageFormat_R16_Uint)
            .initialize(vramTextureLayout16);

        VRAMTextureMemory[i] = Gfx::TextureHeap->Alloc(vramTextureLayout16.getSize(), vramTextureLayout16.getAlignment());
        VRAM8Texture[i].initialize(vramTextureLayout8, Gfx::TextureHeap->MemBlock, VRAMTextureMemory[i].Offset);
        VRAM16Texture[i].initialize(vramTextureLayout16, Gfx::TextureHeap->MemBlock, VRAMTextureMemory[i].Offset);
    }
    dk::ImageLayout paletteTextureLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setType(DkImageType_Buffer)
        .setDimensions(paletteMemory_UnitSize/2)
        .setFormat(DkImageFormat_R16_Uint)
        .initialize(paletteTextureLayout);
    PaletteTextureMemory = Gfx::TextureHeap->Alloc(paletteTextureLayout.getSize()*2, paletteTextureLayout.getAlignment());
    for (int i = 0; i < 2; i++)
        PaletteTextures[i].initialize(paletteTextureLayout, Gfx::TextureHeap->MemBlock, PaletteTextureMemory.Offset + i*paletteTextureLayout.getSize());

    dk::ImageLayout directDisplayTextureLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setType(DkImageType_2D)
        .setDimensions(256, 192)
        .setFormat(DkImageFormat_R16_Uint)
        .initialize(directDisplayTextureLayout);
    DirectBitmapTextureMemory = Gfx::TextureHeap->Alloc(directDisplayTextureLayout.getSize()*2, directDisplayTextureLayout.getAlignment());
    for (int i = 0; i < 2; i++)
        DirectBitmapTexture[i].initialize(directDisplayTextureLayout, Gfx::TextureHeap->MemBlock,
            DirectBitmapTextureMemory.Offset + directDisplayTextureLayout.getSize()*i);

    BGOBJTextureMemory = Gfx::TextureHeap->Alloc(intermedFbLayout.getSize(), intermedFbLayout.getAlignment());
    BGOBJTexture.initialize(intermedFbLayout, Gfx::TextureHeap->MemBlock, BGOBJTextureMemory.Offset);

    DisplayCaptureMemory = Gfx::DataHeap->Alloc(256*192*4, 64);

    dk::ImageLayout disabledBGLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setType(DkImageType_2D)
        .setDimensions(8, 8)
        .setFormat(DkImageFormat_R8_Uint)
        .initialize(disabledBGLayout);
    DisabledBGMemory = Gfx::TextureHeap->Alloc(disabledBGLayout.getSize(), disabledBGLayout.getAlignment());
    DisabledBG.initialize(disabledBGLayout, Gfx::TextureHeap->MemBlock, DisabledBGMemory.Offset);

    dk::ImageLayout mosaicTableLayout;
    dk::ImageLayoutMaker{Gfx::Device}
        .setType(DkImageType_2D)
        .setDimensions(256, 16)
        .setFormat(DkImageFormat_R8_Uint)
        .setFlags(DkImageFlags_PitchLinear)
        .setPitchStride(256)
        .initialize(mosaicTableLayout);
    MosaicTableMemory = Gfx::TextureHeap->Alloc(mosaicTableLayout.getSize(), mosaicTableLayout.getAlignment());
    MosaicTable.initialize(mosaicTableLayout, Gfx::TextureHeap->MemBlock, MosaicTableMemory.Offset);

    ImageDescriptors = Gfx::DataHeap->Alloc(sizeof(dk::ImageDescriptor)*descriptorOffset_Count, DK_IMAGE_DESCRIPTOR_ALIGNMENT);
    dk::ImageDescriptor* imageDescriptors = Gfx::DataHeap->CpuAddr<dk::ImageDescriptor>(ImageDescriptors);
    for (u32 i = 0; i < fb_Count*2; i++)
    {
        imageDescriptors[descriptorOffset_IntermedFb+i].initialize(dk::ImageView{IntermedFramebuffers[i]});
        imageDescriptors[descriptorOffset_IntermedFbHiRes+i].initialize(dk::ImageView{IntermedFramebuffersHiRes[i]});
    }
    for (u32 i = 0; i < textureVRAM_Count; i++)
    {
        imageDescriptors[descriptorOffset_VRAM8+i].initialize(dk::ImageView{VRAM8Texture[i]});
        imageDescriptors[descriptorOffset_VRAM16+i].initialize(dk::ImageView{VRAM16Texture[i]});
    }
    for (u32 i = 0; i < 2; i++)
    {
        imageDescriptors[descriptorOffset_Palettes+i].initialize(dk::ImageView{PaletteTextures[i]});
        imageDescriptors[descriptorOffset_DirectBitmap+i].initialize(dk::ImageView{DirectBitmapTexture[i]});
        imageDescriptors[descriptorOffset_OBJWindow+i].initialize(dk::ImageView{OBJWindow[i]});
    }
    imageDescriptors[descriptorOffset_3DFramebuffer].initialize(dk::ImageView{_3DFramebuffer});
    imageDescriptors[descriptorOffset_3DFramebufferHiRes].initialize(dk::ImageView{_3DFramebufferHiRes});
    imageDescriptors[descriptorOffset_DisabledBG].initialize(dk::ImageView{DisabledBG});
    imageDescriptors[descriptorOffset_MosaicTable].initialize(dk::ImageView{MosaicTable});

    SamplerDescriptors = Gfx::DataHeap->Alloc(sizeof(dk::SamplerDescriptor), DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
    Gfx::DataHeap->CpuAddr<dk::SamplerDescriptor>(SamplerDescriptors)->initialize(dk::Sampler{});

    Gfx::LoadShader("romfs:/shaders/FullscreenQuad_vsh.dksh", ShaderFullscreenQuad);
    Gfx::LoadShader("romfs:/shaders/ComposeBGOBJ_fsh.dksh", ShaderComposeBGOBJ);
    Gfx::LoadShader("romfs:/shaders/ComposeBGOBJDirectBitmapOnly_fsh.dksh", ShaderComposeBGOBJDirectBitmapOnly);
    Gfx::LoadShader("romfs:/shaders/ComposeBGOBJShowBitmap_fsh.dksh", ShaderComposeBGOBJShowBitmap);
    Gfx::LoadShader("romfs:/shaders/BGText4bpp_fsh.dksh", ShaderBGText4Bpp[0]);
    Gfx::LoadShader("romfs:/shaders/BGText4bppMosaic_fsh.dksh", ShaderBGText4Bpp[1]);
    Gfx::LoadShader("romfs:/shaders/BGText8bpp_fsh.dksh", ShaderBGText8Bpp[0]);
    Gfx::LoadShader("romfs:/shaders/BGText8bppMosaic_fsh.dksh", ShaderBGText8Bpp[1]);
    Gfx::LoadShader("romfs:/shaders/BGAffine_fsh.dksh", ShaderBGAffine[0]);
    Gfx::LoadShader("romfs:/shaders/BGAffineMosaic_fsh.dksh", ShaderBGAffine[1]);
    Gfx::LoadShader("romfs:/shaders/BGExtendedBitmap8bpp_fsh.dksh", ShaderBGExtendedBitmap8pp[0]);
    Gfx::LoadShader("romfs:/shaders/BGExtendedBitmap8bppMosaic_fsh.dksh", ShaderBGExtendedBitmap8pp[1]);
    Gfx::LoadShader("romfs:/shaders/BGExtendedBitmapDirect_fsh.dksh", ShaderBGExtendedBitmapDirect[0]);
    Gfx::LoadShader("romfs:/shaders/BGExtendedBitmapDirectMosaic_fsh.dksh", ShaderBGExtendedBitmapDirect[1]);
    Gfx::LoadShader("romfs:/shaders/BGExtendedMixed_fsh.dksh", ShaderBGExtendedMixed[0]);
    Gfx::LoadShader("romfs:/shaders/BGExtendedMixedMosaic_fsh.dksh", ShaderBGExtendedMixed[1]);
    Gfx::LoadShader("romfs:/shaders/OBJRegular_vsh.dksh", ShaderOBJRegular);
    Gfx::LoadShader("romfs:/shaders/OBJAffine_vsh.dksh", ShaderOBJAffine);
    Gfx::LoadShader("romfs:/shaders/OBJ4bpp_fsh.dksh", ShaderOBJ4bpp);
    Gfx::LoadShader("romfs:/shaders/OBJ8bpp_fsh.dksh", ShaderOBJ8bpp);
    Gfx::LoadShader("romfs:/shaders/OBJBitmap_fsh.dksh", ShaderOBJBitmap);
    Gfx::LoadShader("romfs:/shaders/OBJWindow4bpp_fsh.dksh", ShaderOBJWindow4bpp);
    Gfx::LoadShader("romfs:/shaders/OBJWindow8bpp_fsh.dksh", ShaderOBJWindow8bpp);

    BGUniformMemory = Gfx::DataHeap->Alloc(BGUniformSize, DK_UNIFORM_BUF_ALIGNMENT);
    OBJUniformMemory = Gfx::DataHeap->Alloc(OBJUniformSize, DK_UNIFORM_BUF_ALIGNMENT);
    ComposeUniformMemory = Gfx::DataHeap->Alloc(ComposeUniformSize, DK_UNIFORM_BUF_ALIGNMENT);

    {
        CmdMem.Begin(EmuCmdBuf);
        u8 disabledBGData[8*8] = {0};
        UploadBuf.UploadAndCopyTexture(EmuCmdBuf, DisabledBG, disabledBGData, 0, 0, 8, 8, 8);
        u8 mosaicTable[256*16] = {0};
        for (int m = 0; m < 16; m++)
        {
            for (int x = 0; x < 256; x++)
            {
                int offset = x - (x % (m+1));
                mosaicTable[m*256+x] = offset;
            }
        }
        UploadBuf.UploadAndCopyTexture(EmuCmdBuf, MosaicTable, mosaicTable, 0, 0, 256, 16, 256);
        EmuQueue.submitCommands(CmdMem.End(EmuCmdBuf));
        EmuQueue.waitIdle();
        UploadBuf.LastFlushBuffer = 0;
    }
}

DekoRenderer::~DekoRenderer()
{
}

template <u32 Size>
void DekoRenderer::UploadVRAM(const NonStupidBitField<Size>& dirty, u8* src, DkGpuAddr dst)
{
    u32 start = dirty.Min()*GPU::VRAMDirtyGranularity;
    u32 size = ((dirty.Max()+1)*GPU::VRAMDirtyGranularity)-start;
    assert(start < Size*GPU::VRAMDirtyGranularity);
    assert(start + size <= Size*GPU::VRAMDirtyGranularity);
    UploadBuf.UploadAndCopyData(EmuCmdBuf, dst + start, src + start, size);
}

template <u32 Size>
void DekoRenderer::UploadPalVRAM(const NonStupidBitField<Size>& dirty, u8* dataSrc, u32& dst, DkGpuAddr& src, u32& size)
{
    dst = dirty.Min()*GPU::VRAMDirtyGranularity;
    size = ((dirty.Max()+1)*GPU::VRAMDirtyGranularity)-dst;
    assert(dst < Size*GPU::VRAMDirtyGranularity);
    assert(dst + size <= Size*GPU::VRAMDirtyGranularity);
    src = UploadBuf.UploadData(EmuCmdBuf, size, dataSrc + dst);
}

void DekoRenderer::OpenCmdBuf()
{
    if (!CmdBufOpen)
    {
        CmdMem.Begin(EmuCmdBuf);
        DkViewport viewport = {0.f, 0.f, 256.f, 192.f, 0.f, 1.f};
        EmuCmdBuf.setViewports(0, {viewport, viewport});
        EmuCmdBuf.bindSamplerDescriptorSet(Gfx::DataHeap->GpuAddr(SamplerDescriptors), 1);
        EmuCmdBuf.bindImageDescriptorSet(Gfx::DataHeap->GpuAddr(ImageDescriptors), descriptorOffset_Count);
        EmuCmdBuf.bindColorWriteState(dk::ColorWriteState{});
        CmdBufOpen = true;
    }
}

void DekoRenderer::Reset()
{
    for (int i = 0; i < 2; i++)
    {
        LastDispCnt[i] = 0;
        LastOBJDispCnt[i] = 0;

        OBJBatchFirstLine[i] = 0;
        OBJBatchLinesCount[i] = 0;

        LastBlendCnt[i] = 0;
        LastEVA[i] = 0;
        LastEVB[i] = 0;
        LastEVY[i] = 0;
        LastMasterBrightness[i] = 0;
        LastForceBlank[i] = false;
        DirectBitmapNeeded[i] = false;
        LastBGMosaicSizeX[i] = 0;
        LastBGMosaicYMax[i] = 0;
        LastOBJMosaicSizeX[i] = 0;
        LastOBJMosaicSizeY[i] = 0;
        BGOBJRedrawn[i] = 0;
        BGHiResValid[i] = 0;
        OBJHiResValid[i] = false;
        ClearOBJHiResLines(i);
        OBJCompositionDirty[i] = false;
        OBJWindowEmpty[i] = true;
        ComposeRegions[i].clear();

        for (int j = 0; j < 4; j++)
        {
            ClearBGHiResLines(i, j);
            LastBGCnt[i][j] = 0;
            LastBGXPos[i][j] = 0;
            LastBGYPos[i][j] = 0;

            BGBatchFirstLine[i][j] = 0;
            BGBatchLinesCount[i][j] = 0;
        }

        for (int j = 0; j < 2; j++)
        {
            LastBGRotA[i][j] = 0;
            LastBGRotB[i][j] = 0;
            LastBGRotC[i][j] = 0;
            LastBGRotD[i][j] = 0;
            AffineChangedMidframe[i][j] = false;
        }
    }

    CaptureLatch = false;
    CaptureCnt = 0;
    memset(DisplayCaptureSourceB, 0, sizeof(DisplayCaptureSourceB));
    memset(DisplayCaptureLine, 0xFF, sizeof(DisplayCaptureLine));
    memset(DisplayCaptureSourceBAddr, 0, sizeof(DisplayCaptureSourceBAddr));
    memset(DisplayCaptureSourceBVRAMBank, 0xFF, sizeof(DisplayCaptureSourceBVRAMBank));

    memset(OAMShadow, 0, sizeof(OAMShadow));
}

void DekoRenderer::ClearBGHiResLines(u32 unit, u32 bg)
{
    memset(BGHiResLineValid[unit][bg], 0, sizeof(BGHiResLineValid[unit][bg]));
    BGHiResValid[unit] &= ~(1U << bg);
}

void DekoRenderer::SetBGHiResLines(u32 unit, u32 bg, u32 firstLine, u32 linesCount, bool valid)
{
    if (firstLine >= NativeHeight || linesCount == 0)
        return;

    if (firstLine + linesCount > NativeHeight)
        linesCount = NativeHeight - firstLine;

    for (u32 line = firstLine; line < firstLine + linesCount; line++)
    {
        u64 bit = 1ULL << (line & 63);
        u64& word = BGHiResLineValid[unit][bg][line >> 6];
        if (valid)
            word |= bit;
        else
            word &= ~bit;
    }

    bool anyValid = false;
    for (u32 word = 0; word < HiResLineValidWords; word++)
        anyValid |= BGHiResLineValid[unit][bg][word] != 0;

    if (anyValid)
        BGHiResValid[unit] |= 1U << bg;
    else
        BGHiResValid[unit] &= ~(1U << bg);
}

bool DekoRenderer::BGHiResLinesValid(u32 unit, u32 bg, u32 firstLine, u32 linesCount) const
{
    if ((BGHiResValid[unit] & (1U << bg)) == 0 || firstLine >= NativeHeight || linesCount == 0)
        return false;

    if (firstLine + linesCount > NativeHeight)
        return false;

    for (u32 line = firstLine; line < firstLine + linesCount; line++)
    {
        u64 bit = 1ULL << (line & 63);
        if ((BGHiResLineValid[unit][bg][line >> 6] & bit) == 0)
            return false;
    }

    return true;
}

void DekoRenderer::ClearOBJHiResLines(u32 unit)
{
    memset(OBJHiResLineValid[unit], 0, sizeof(OBJHiResLineValid[unit]));
    OBJHiResValid[unit] = false;
}

void DekoRenderer::SetOBJHiResLines(u32 unit, u32 firstLine, u32 linesCount, bool valid)
{
    if (firstLine >= NativeHeight || linesCount == 0)
        return;

    if (firstLine + linesCount > NativeHeight)
        linesCount = NativeHeight - firstLine;

    for (u32 line = firstLine; line < firstLine + linesCount; line++)
    {
        u64 bit = 1ULL << (line & 63);
        u64& word = OBJHiResLineValid[unit][line >> 6];
        if (valid)
            word |= bit;
        else
            word &= ~bit;
    }

    bool anyValid = false;
    for (u32 word = 0; word < HiResLineValidWords; word++)
        anyValid |= OBJHiResLineValid[unit][word] != 0;

    OBJHiResValid[unit] = anyValid;
}

bool DekoRenderer::OBJHiResLinesValid(u32 unit, u32 firstLine, u32 linesCount) const
{
    if (!OBJHiResValid[unit] || firstLine >= NativeHeight || linesCount == 0)
        return false;

    if (firstLine + linesCount > NativeHeight)
        return false;

    for (u32 line = firstLine; line < firstLine + linesCount; line++)
    {
        u64 bit = 1ULL << (line & 63);
        if ((OBJHiResLineValid[unit][line >> 6] & bit) == 0)
            return false;
    }

    return true;
}

void DekoRenderer::DrawScanline(u32 line, Unit* unit)
{
    CurUnit = unit;

    int n3dline = line;
    line = GPU::VCount;

    int num = CurUnit->Num;

    bool forceblank = false;

    if (line > NativeHeight) forceblank = true;
    if (CurUnit->Num && !CurUnit->Enabled) forceblank = true;

    OpenCmdBuf();

    bool uploadBarrier = false;
    {
        u32 bgmask = 0;
        bool compositionDirty = n3dline == 0 || OBJCompositionDirty[num];
        ComposeRegion composeRegion = {0};

        for (int i = 0; i < 4; i++)
        {
            if (LastBGXPos[num][i] != CurUnit->BGXPos[i] ||
                LastBGYPos[num][i] != CurUnit->BGYPos[i])
            {
                LastBGXPos[num][i] = CurUnit->BGXPos[i];
                LastBGYPos[num][i] = CurUnit->BGYPos[i];
                if (BGBatchFirstLine[num][i] == -1)
                    bgmask |= 1 << i;
            }
        }
        for (int i = 0; i < 2; i++)
        {
            bool originChanged = LastBGXRef[num][i] != CurUnit->BGXRef[i]
                || LastBGYRef[num][i] != CurUnit->BGYRef[i];
            bool matrixChanged = LastBGRotA[num][i] != CurUnit->BGRotA[i]
                || LastBGRotB[num][i] != CurUnit->BGRotB[i]
                || LastBGRotC[num][i] != CurUnit->BGRotC[i]
                || LastBGRotD[num][i] != CurUnit->BGRotD[i];

            AffineChangedMidframe[num][i] |= (originChanged || matrixChanged) && n3dline > 0;

            if (originChanged || matrixChanged)
            {
                LastBGXRef[num][i] = CurUnit->BGXRef[i];
                LastBGYRef[num][i] = CurUnit->BGYRef[i];
                LastBGRotA[num][i] = CurUnit->BGRotA[i];
                LastBGRotB[num][i] = CurUnit->BGRotB[i];
                LastBGRotC[num][i] = CurUnit->BGRotC[i];
                LastBGRotD[num][i] = CurUnit->BGRotD[i];
                if (BGBatchFirstLine[num][i+2] == -1)
                    bgmask |= 1 << (i+2);
            }
        }
    
        bool outputForceBlankChanged = LastForceBlank[num] != forceblank;
        if (LastBlendCnt[num] != CurUnit->BlendCnt
            || LastMasterBrightness[num] != CurUnit->MasterBrightness
            || LastEVA[num] != CurUnit->EVA
            || LastEVB[num] != CurUnit->EVB
            || LastEVY[num] != CurUnit->EVY
            || outputForceBlankChanged)
        {
            LastBlendCnt[num] = CurUnit->BlendCnt;
            LastMasterBrightness[num] = CurUnit->MasterBrightness;
            LastEVA[num] = CurUnit->EVA;
            LastEVB[num] = CurUnit->EVB;
            LastEVY[num] = CurUnit->EVY;
            LastForceBlank[num] = forceblank;
            compositionDirty = true;
        }
        if (outputForceBlankChanged)
            bgmask = 0xF;

        // Bit 3 switches main-engine BG0 between 2D and 3D layer sources.
        if ((LastDispCnt[num] ^ CurUnit->DispCnt) & 0xFF030F8F)
            bgmask = 0xF;
        if ((LastDispCnt[num] ^ CurUnit->DispCnt) & 0x31F8C)
            compositionDirty = true;

        bool mosaicChanged = LastBGMosaicYMax[num] != CurUnit->BGMosaicYMax
            || LastBGMosaicSizeX[num] != CurUnit->BGMosaicSize[0];
        for (int i = 0; i < 4; i++)
        {
            if (((LastBGCnt[num][i] ^ CurUnit->BGCnt[i]) & 0xFFFC) || (LastBGCnt[num][i] & 0x0040 && mosaicChanged))
                bgmask |= 1 << i;
            if ((LastBGCnt[num][i] ^ CurUnit->BGCnt[i]) & 0x3)
                compositionDirty = true;
        }

        u32 paletteDirty = (GPU::PaletteDirty >> CurUnit->Num*2) & 0x3;

        if (num == 0)
        {
            auto bgDirty = GPU::VRAMDirty_ABG.DeriveState(GPU::VRAMMap_ABG);

            if (bgDirty)
                bgmask = 0xF;

            FlushBGDraw(n3dline, bgmask);

            if (GPU::MakeVRAMFlat_ABGCoherent(bgDirty))
            {
                UploadVRAM(bgDirty, GPU::VRAMFlat_ABG, Gfx::TextureHeap->GpuAddr(VRAMTextureMemory[textureVRAM_ABG]));
                uploadBarrier = true;
            }
            auto bgExtPalDirty = GPU::VRAMDirty_ABGExtPal.DeriveState(GPU::VRAMMap_ABGExtPal);
            if (GPU::MakeVRAMFlat_ABGExtPalCoherent(bgExtPalDirty))
            {
                UploadPalVRAM(bgExtPalDirty, GPU::VRAMFlat_ABGExtPal, composeRegion.BGExtPalDst, composeRegion.BGExtPalSrc, composeRegion.BGExtPalSize);
                compositionDirty = true;
            }

            auto objExtPalDirty = GPU::VRAMDirty_AOBJExtPal.DeriveState(&GPU::VRAMMap_AOBJExtPal);
            if (GPU::MakeVRAMFlat_AOBJExtPalCoherent(objExtPalDirty))
            {
                UploadPalVRAM(objExtPalDirty, GPU::VRAMFlat_AOBJExtPal, composeRegion.OBJExtPalDst, composeRegion.OBJExtPalSrc, composeRegion.OBJExtPalSize);
                compositionDirty = true;
            }
        }
        else
        {
            auto bgDirty = GPU::VRAMDirty_BBG.DeriveState(GPU::VRAMMap_BBG);

            if (bgDirty)
                bgmask = 0xF;

            FlushBGDraw(n3dline, bgmask);

            if (GPU::MakeVRAMFlat_BBGCoherent(bgDirty))
            {
                UploadVRAM(bgDirty, GPU::VRAMFlat_BBG, Gfx::TextureHeap->GpuAddr(VRAMTextureMemory[textureVRAM_BBG]));
                uploadBarrier = true;
            }
            auto bgExtPalDirty = GPU::VRAMDirty_BBGExtPal.DeriveState(GPU::VRAMMap_BBGExtPal);
            if (GPU::MakeVRAMFlat_BBGExtPalCoherent(bgExtPalDirty))
            {
                UploadPalVRAM(bgExtPalDirty, GPU::VRAMFlat_BBGExtPal, composeRegion.BGExtPalDst, composeRegion.BGExtPalSrc, composeRegion.BGExtPalSize);
                compositionDirty = true;
            }

            auto objExtPalDirty = GPU::VRAMDirty_BOBJExtPal.DeriveState(&GPU::VRAMMap_BOBJExtPal);
            if (GPU::MakeVRAMFlat_BOBJExtPalCoherent(objExtPalDirty))
            {
                UploadPalVRAM(objExtPalDirty, GPU::VRAMFlat_BOBJExtPal, composeRegion.OBJExtPalDst, composeRegion.OBJExtPalSrc, composeRegion.OBJExtPalSize);
                compositionDirty = true;
            }
        }

        if (paletteDirty)
        {
            composeRegion.StdPalDst = (paletteDirty & 1) ? 0 : 512;
            composeRegion.StdPalSize = paletteDirty == 0x3 ? 1024 : 512;
            composeRegion.StdPalSrc = UploadBuf.UploadData(EmuCmdBuf, composeRegion.StdPalSize, &GPU::Palette[composeRegion.StdPalDst + (num ? 1024 : 0)]);
            GPU::PaletteDirty &= ~(0x3 << num*2);
            compositionDirty = true;
        }

        if (bgmask)
            compositionDirty = true;

        LastDispCnt[num] = CurUnit->DispCnt;
        for (int i = 0; i < 4; i++)
            LastBGCnt[num][i] = CurUnit->BGCnt[i];
        LastBGMosaicSizeX[num] = CurUnit->BGMosaicSize[0];
        LastBGMosaicYMax[num] = CurUnit->BGMosaicYMax;

        if (compositionDirty)
        {
            FillinCurComposeRegion(composeRegion, forceblank);
            ComposeRegions[num].push_back(composeRegion);
            OBJCompositionDirty[num] = false;
        }
    }

    if (n3dline == 0 && CurUnit->Num == 0)
    {
        CurUnit->CaptureLatch = false;
        CaptureLatch = false;
        if (!forceblank && (CurUnit->CaptureCnt & (1 << 31)))
        {
            CurUnit->CaptureLatch = true;
            CaptureLatch = true;
            CaptureCnt = CurUnit->CaptureCnt;
            memset(DisplayCaptureLine, 0xFF, sizeof(DisplayCaptureLine));
            memset(DisplayCaptureSourceBAddr, 0, sizeof(DisplayCaptureSourceBAddr));
            memset(DisplayCaptureSourceBVRAMBank, 0xFF, sizeof(DisplayCaptureSourceBVRAMBank));
        }
    }

    if (CurUnit->Num == 0 && CaptureLatch)
        DisplayCaptureLine[n3dline] = line;

    if (CurUnit->Num == 0 && CaptureLatch && ((CaptureCnt >> 29) & 0x3) != 0)
    {
        u16* dst = &DisplayCaptureSourceB[n3dline*NativeWidth];
        DisplayCaptureSourceBAddr[n3dline] = 0;
        DisplayCaptureSourceBVRAMBank[n3dline] = 0xFF;
        if (CaptureCnt & (1<<25))
        {
            memcpy(dst, CurUnit->DispFIFOBuffer, NativeWidth*2);
        }
        else
        {
            u32 dispCnt = CurUnit->DispCnt;
            u16* src = NULL;
            u32 srcvram = (dispCnt >> 18) & 0x3;
            if (GPU::VRAMMap_LCDC & (1<<srcvram))
                src = (u16*)GPU::VRAM[srcvram];

            u32 srcBaddr = line * NativeWidth;
            if (((dispCnt >> 16) & 0x3) != 2)
                srcBaddr += ((CaptureCnt >> 26) & 0x3) << 14;
            srcBaddr &= 0xFFFF;
            DisplayCaptureSourceBAddr[n3dline] = srcBaddr;

            if (src)
            {
                DisplayCaptureSourceBVRAMBank[n3dline] = srcvram;
                u32 firstCopy = NativeWidth;
                if (srcBaddr + firstCopy > 0x10000)
                    firstCopy = 0x10000 - srcBaddr;
                memcpy(dst, &src[srcBaddr], firstCopy*2);
                if (firstCopy < NativeWidth)
                    memcpy(&dst[firstCopy], src, (NativeWidth - firstCopy)*2);
            }
            else
            {
                memset(dst, 0, NativeWidth*2);
            }
        }
    }

    /*if (CurUnit->Num == 0)
    {
        u32* _3dline = GPU3D::GetLine(n3dline);
        for (u32 i = 0; i < 256; i++)
        {
            _3DBuffer[n3dline*256+i] = (_3dline[i] >> 24) ? (_3dline[i] |0x40000000) : 0;
        }
    }*/

    if (!forceblank)
    {
        u32 dispmode = CurUnit->DispCnt >> 16;
        dispmode &= (num ? 0x1 : 0x3);

        switch (dispmode)
        {
        case 0:
            memset(&DirectBitmap[num][n3dline*256], 0xFF, 256*2);
            DirectBitmapNeeded[num] = true;
            break;
        case 2:
            {
                DirectBitmapNeeded[num] = true;
                u32 vrambank = (CurUnit->DispCnt >> 18) & 0x3;
                if (GPU::VRAMMap_LCDC & (1<<vrambank))
                {
                    u16* vram = (u16*)GPU::VRAM[vrambank];
                    vram = &vram[line * 256];

                    memcpy(&DirectBitmap[num][n3dline*256], vram, 256*2);
                }
                else
                {
                    memset(&DirectBitmap[num][n3dline*256], 0, 256*2);
                }
            }
            break;
        case 3:
            DirectBitmapNeeded[num] = true;
            memcpy(&DirectBitmap[num][n3dline*256], CurUnit->DispFIFOBuffer, 256*2);
            break;
        }
    }
    else
    {
        DirectBitmapNeeded[num] = true;
        memset(&DirectBitmap[num][n3dline*256], 0xFF, 256*2);
    }

    if (!forceblank)
    {
        DrawScanline_BGOBJ(line, n3dline);
    }
    else
    {
        for (u32 i = 0; i < 4; i++)
            BGState[num][i] = bgState_Disable;
    }

    for (u32 i = 0; i < 4; i++)
    {
        BGBatchLinesCount[num][i]++;
    }
    ComposeRegions[num][ComposeRegions[num].size()-1].LinesCount++;

    if (n3dline == 191)
    {
        if (DirectBitmapNeeded[num])
        {
            UploadBuf.UploadAndCopyTexture(EmuCmdBuf, DirectBitmapTexture[num], (u8*)DirectBitmap[CurUnit->Num], 0, 0, 256, 192, 256*2);
            uploadBarrier = true;

            DirectBitmapNeeded[num] = false;
        }

        /*if (num == 0)
        {
            UploadBuf.UploadAndCopyTexture(EmuCmdBuf, _3DFramebuffer, (u8*)_3DBuffer, 0, 0, 256, 192, 256*4);
            uploadBarrier = true;
        }*/
    }

    if (uploadBarrier)
        EmuCmdBuf.barrier(DkBarrier_Full, DkInvalidateFlags_Image);

    if (n3dline == 191)
    {
        u32 renderFull = 0;
        for (int i = 0; i < 4; i++)
        {
            if ((BGBatchFirstLine[num][i] == 0 && BGBatchLinesCount[num][i] == 192)
                || BGBatchFirstLine[num][i] == -1)
                renderFull |= 1 << i;
        }
        // flush all backgrounds
        FlushBGDraw(n3dline + 1, 0xF);

        for (int i = 0; i < 2; i++)
            BGBatchFirstLine[num][i] = (renderFull & (1<<i)) ? -1 : 0;
        for (int i = 2; i < 4; i++)
        {
            // if the affine reference is changed midframe the rest of the frame
            // will be rendered based of that value starting from that line
            // but during the next frame that value will be the value of the internal register
            // on the first line
            // so that frame will be different
            BGBatchFirstLine[num][i] = (renderFull & (1<<i)) && !AffineChangedMidframe[num][i-2] ? -1 : 0;
            AffineChangedMidframe[num][i-2] = false;
        }

        if (unit->Num == 0)
        {
            EmuCmdBuf.waitFence(FramebufferPresented[GPU::FrontBuffer^1]);
        }

        ComposeBGOBJ();

        if (unit->Num == 1)
        {
            EmuCmdBuf.signalFence(FramebufferReady[GPU::FrontBuffer^1]);
            EmuQueue.submitCommands(CmdMem.End(EmuCmdBuf));
            EmuQueue.flush();
            UploadBuf.LastFlushBuffer = 0;

            CmdBufOpen = false;

            if (CaptureLatch)
            {
                //u64 startTime = armGetSystemTick();
                /*DkResult result = */DisplayCaptureFence.wait();
                //printf("wait result %d %fms\n", result, armTicksToNs(armGetSystemTick()-startTime)*0.000001f);
                DoCapture();
            }
        }
    }
}

void DekoRenderer::DrawSprites(u32 line, Unit* unit)
{
    CurUnit = unit;

    int num = unit->Num;

    OpenCmdBuf();

    bool oamDirty = false;
    if (GPU::OAMDirty & (1 << num))
    {
        oamDirty = true;
        GPU::OAMDirty &= ~(1 << num);
    }

    // OBJ address and palette mode bits affect the generated sprite layer.
    const u32 objDispCntMask = 0x80700070;
    bool objFmtChanged = (LastOBJDispCnt[num] ^ CurUnit->DispCnt) & objDispCntMask;
    bool objMosaicChanged = LastOBJMosaicSizeX[num] != CurUnit->OBJMosaicSize[0]
        || LastOBJMosaicSizeY[num] != CurUnit->OBJMosaicSize[1];

    bool uploadBarrier = false;
    if (num == 0)
    {
        auto objDirty = GPU::VRAMDirty_AOBJ.DeriveState(GPU::VRAMMap_AOBJ);

        if (oamDirty || objDirty || objFmtChanged || objMosaicChanged)
            FlushOBJDraw(line);

        if (GPU::MakeVRAMFlat_AOBJCoherent(objDirty))
        {
            UploadVRAM(objDirty, GPU::VRAMFlat_AOBJ, Gfx::TextureHeap->GpuAddr(VRAMTextureMemory[textureVRAM_AOBJ]));
            uploadBarrier = true;
        }
    }
    else
    {
        auto objDirty = GPU::VRAMDirty_BOBJ.DeriveState(GPU::VRAMMap_BOBJ);

        if (oamDirty || objDirty || objFmtChanged || objMosaicChanged)
            FlushOBJDraw(line);

        if (GPU::MakeVRAMFlat_BOBJCoherent(objDirty))
        {
            UploadVRAM(objDirty, GPU::VRAMFlat_BOBJ, Gfx::TextureHeap->GpuAddr(VRAMTextureMemory[textureVRAM_BOBJ]));
            uploadBarrier = true;
        }
    }

    if (uploadBarrier)
        EmuCmdBuf.barrier(DkBarrier_Full, DkInvalidateFlags_Image);

    if (oamDirty)
    {
        memcpy(&OAMShadow[num ? 0x400 : 0], &GPU::OAM[num ? 0x400 : 0], 0x400);
    }
    LastOBJMosaicSizeX[num] = CurUnit->OBJMosaicSize[0];
    LastOBJMosaicSizeY[num] = CurUnit->OBJMosaicSize[1];
    LastOBJDispCnt[num] = CurUnit->DispCnt;

    OBJBatchLinesCount[num]++;

    if (line == 191)
    {
        bool drawFull = (OBJBatchFirstLine[num] == 0 && OBJBatchLinesCount[num] == 192)
            || OBJBatchFirstLine[num] == -1;
        FlushOBJDraw(line+1);

        OBJBatchFirstLine[num] = drawFull ? -1 : 0;
    }
}

u32 frame = 0;

void DekoRenderer::DoCapture()
{
    u32 width, height;
    switch ((CaptureCnt >> 20) & 0x3)
    {
    case 0: width = 128; height = 128; break;
    case 1: width = 256; height = 64;  break;
    case 2: width = 256; height = 128; break;
    case 3: width = 256; height = 192; break;
    }

    u32 dstvram = (CaptureCnt >> 16) & 0x3;

    // TODO: confirm this
    // it should work like VRAM display mode, which requires VRAM to be mapped to LCDC
    if (!(GPU::VRAMMap_LCDC & (1<<dstvram)))
        return;

    u16* dst = (u16*)GPU::VRAM[dstvram];
    u32 captureBaseAddr = (((CaptureCnt >> 18) & 0x3) << 14);

    u8* srcA = Gfx::DataHeap->CpuAddr<u8>(DisplayCaptureMemory);

    u32 source = (CaptureCnt >> 29) & 0x3;
    u16* srcB = source != 0 ? DisplayCaptureSourceB : NULL;

    captureBaseAddr &= 0xFFFF;

    static_assert(GPU::VRAMDirtyGranularity == 512, "");
    auto markCaptureDirty = [&](u32 dstpos, u32 pixels)
    {
        u32 byteAddr = (dstpos * 2) & 0x1FFFF;
        u32 bytesLeft = pixels * 2;
        while (bytesLeft > 0)
        {
            u32 chunk = bytesLeft;
            u32 bankBytesLeft = 0x20000 - byteAddr;
            if (chunk > bankBytesLeft)
                chunk = bankBytesLeft;

            u32 firstBlock = byteAddr / GPU::VRAMDirtyGranularity;
            u32 lastBlock = (byteAddr + chunk - 1) / GPU::VRAMDirtyGranularity;
            for (u32 block = firstBlock; block <= lastBlock; block++)
                GPU::VRAMDirty[dstvram][block] = true;

            bytesLeft -= chunk;
            byteAddr = 0;
        }
    };
    markCaptureDirty(captureBaseAddr, width*height);

    auto getCaptureDstAddr = [&](u32 outputLine, u32& dstaddr)
    {
        u32 captureLine = DisplayCaptureLine[outputLine];
        if (captureLine >= height)
            return false;

        dstaddr = (captureBaseAddr + captureLine * width) & 0xFFFF;
        return true;
    };

    u64 captureWritten[0x10000 / 64] = {};
    auto isCaptureWritten = [&](u32 pos)
    {
        pos &= 0xFFFF;
        return (captureWritten[pos >> 6] & (1ULL << (pos & 63))) != 0;
    };
    auto markCaptureWritten = [&](u32 dstpos, u32 pixels)
    {
        dstpos &= 0xFFFF;
        while (pixels > 0)
        {
            u32 bit = dstpos & 63;
            u32 amount = 64 - bit;
            u32 bankLeft = 0x10000 - dstpos;
            if (amount > pixels) amount = pixels;
            if (amount > bankLeft) amount = bankLeft;

            u64 mask = amount == 64 ? ~0ULL : ((1ULL << amount) - 1ULL) << bit;
            captureWritten[dstpos >> 6] |= mask;

            dstpos = (dstpos + amount) & 0xFFFF;
            pixels -= amount;
        }
    };
    auto loadSourceBPixel = [&](u32 line, u32 x)
    {
        if (DisplayCaptureSourceBVRAMBank[line] == dstvram)
        {
            u32 vramPos = (DisplayCaptureSourceBAddr[line] + x) & 0xFFFF;
            if (isCaptureWritten(vramPos))
                return dst[vramPos];
        }

        return DisplayCaptureSourceB[line*NativeWidth + x];
    };

    u32 eva = CaptureCnt & 0x1F;
    u32 evb = (CaptureCnt >> 8) & 0x1F;

    if (eva > 16) eva = 16;
    if (evb > 16) evb = 16;
    if (source >= 2)
    {
        if (!srcB)
            evb = 0;

        if (eva == 16 && evb == 0)
            source = 0;
        if (eva == 0 && evb == 16)
            source = 1;
    }

    //printf("capturing %d %dx%d to %d (eva %d | evb %d) %p %p\n", source, width, height, dstvram, eva, evb, srcA, srcB);

    auto blendCapturePixel = [&](u32& dstpos, u32 line, u32 x)
    {
        u8* pixelA = &srcA[(x + line * NativeWidth) * 4];
        bool alphaA = pixelA[3] != 0;
        u16 pixelB = loadSourceBPixel(line, x);
        bool alphaB = (pixelB & 0x8000) != 0;

        u32 rA = alphaA ? (pixelA[0] >> 1) : 0;
        u32 gA = alphaA ? (pixelA[1] >> 1) : 0;
        u32 bA = alphaA ? (pixelA[2] >> 1) : 0;

        u32 rB = alphaB ? (pixelB & 0x1F) : 0;
        u32 gB = alphaB ? ((pixelB >> 5) & 0x1F) : 0;
        u32 bB = alphaB ? ((pixelB >> 10) & 0x1F) : 0;

        u32 r = ((rA * eva) + (rB * evb)) >> 4;
        u32 g = ((gA * eva) + (gB * evb)) >> 4;
        u32 b = ((bA * eva) + (bB * evb)) >> 4;
        u32 alpha = ((eva != 0 && alphaA) || (evb != 0 && alphaB)) ? 0x8000 : 0;

        if (r > 0x1F) r = 0x1F;
        if (g > 0x1F) g = 0x1F;
        if (b > 0x1F) b = 0x1F;

        dst[dstpos] = r | (g << 5) | (b << 10) | alpha;
        markCaptureWritten(dstpos, 1);
        dstpos = (dstpos + 1) & 0xFFFF;
    };

    switch (source)
    {
    case 0: // source A
        {
            for (u32 j = 0; j < NativeHeight; j++)
            {
                u32 dstaddr;
                if (!getCaptureDstAddr(j, dstaddr))
                    continue;

                for (u32 i = 0; i < width; i += 16)
                {
                    uint8x16x4_t pixels = vld4q_u8(&srcA[(i + j * 256) * 4]);

                    uint8x16_t low = vbslq_u8(vdupq_n_u8(0x1F), vshrq_n_u8(pixels.val[0], 1), vshlq_n_u8(pixels.val[1], 4));
                    uint8x16_t high = vbslq_u8(vdupq_n_u8(0x3), vshrq_n_u8(pixels.val[1], 4), vshlq_n_u8(pixels.val[2], 1));

                    uint8x16_t alpha = vtstq_u8(pixels.val[3], pixels.val[3]);
                    high = vbslq_u8(vdupq_n_u8(0x80), alpha, high);

                    u32 writePos = dstaddr;
                    vst2q_u8((u8*)&dst[dstaddr], {low, high});
                    markCaptureWritten(writePos, 16);
                    dstaddr = (dstaddr + 16) & 0xFFFF;
                }
            }
        }
        break;

    case 1: // source B
        {
            for (u32 j = 0; j < NativeHeight; j++)
            {
                u32 dstaddr;
                if (!getCaptureDstAddr(j, dstaddr))
                    continue;

                for (u32 i = 0; i < width;)
                {
                    u32 copyAmount = width - i;
                    u32 dstLeft = 0x10000 - dstaddr;
                    if (copyAmount > dstLeft) copyAmount = dstLeft;

                    if (DisplayCaptureSourceBVRAMBank[j] == dstvram)
                    {
                        for (u32 k = 0; k < copyAmount; k++)
                        {
                            dst[dstaddr] = loadSourceBPixel(j, i + k);
                            markCaptureWritten(dstaddr, 1);
                            dstaddr = (dstaddr + 1) & 0xFFFF;
                        }
                    }
                    else if (srcB)
                    {
                        u32 writePos = dstaddr;
                        memcpy(&dst[dstaddr], &srcB[j*NativeWidth + i], copyAmount*2);
                        markCaptureWritten(writePos, copyAmount);
                        dstaddr = (dstaddr + copyAmount) & 0xFFFF;
                    }
                    else
                    {
                        u32 writePos = dstaddr;
                        memset(&dst[dstaddr], 0, copyAmount*2);
                        markCaptureWritten(writePos, copyAmount);
                        dstaddr = (dstaddr + copyAmount) & 0xFFFF;
                    }

                    i += copyAmount;
                }
            }
        }
        break;

    case 2: // sources A+B
    case 3:
        {
            if (srcB)
            {
                uint8x16_t evaMask = vdupq_n_u8(eva ? 0xFF : 0);
                uint8x16_t evbMask = vdupq_n_u8(evb ? 0xFF : 0);
                for (u32 j = 0; j < NativeHeight; j++)
                {
                    u32 dstaddr;
                    if (!getCaptureDstAddr(j, dstaddr))
                        continue;

                    bool sourceBMayOverlap = DisplayCaptureSourceBVRAMBank[j] == dstvram;
                    if (sourceBMayOverlap)
                    {
                        for (u32 i = 0; i < width; i++)
                            blendCapturePixel(dstaddr, j, i);
                        continue;
                    }

                    for (u32 i = 0; i < width; i += 16)
                    {
                        uint8x16x4_t pixelsA = vld4q_u8(&srcA[(i + j * 256) * 4]);
                        uint8x16x2_t pixelsB = vld2q_u8((u8*)&srcB[j*NativeWidth + i]);
                        uint8x16_t alphaA = vtstq_u8(pixelsA.val[3], pixelsA.val[3]);
                        uint8x16_t alphaB = vtstq_u8(pixelsB.val[1], vdupq_n_u8(0x80));

                        pixelsA.val[0] = vandq_u8(vshrq_n_u8(pixelsA.val[0], 1), alphaA);
                        pixelsA.val[1] = vandq_u8(vshrq_n_u8(pixelsA.val[1], 1), alphaA);
                        pixelsA.val[2] = vandq_u8(vshrq_n_u8(pixelsA.val[2], 1), alphaA);

                        uint8x16_t redB = vandq_u8(vandq_u8(pixelsB.val[0], vdupq_n_u8(0x1F)), alphaB);
                        uint8x16_t greenB = vandq_u8(vbslq_u8(vdupq_n_u8(0xE7), vshrq_n_u8(pixelsB.val[0], 5), vshlq_n_u8(pixelsB.val[1], 3)), alphaB);
                        uint8x16_t blueB = vandq_u8(vandq_u8(vshrq_n_u8(pixelsB.val[1], 2), vdupq_n_u8(0x1F)), alphaB);

                        uint8x16_t veva = vdupq_n_u8(eva);
                        uint8x16_t vevb = vdupq_n_u8(evb);

                        uint16x8_t finalRLo = vmull_u8(vget_low_u8(pixelsA.val[0]), vget_low_u8(veva));
                        uint16x8_t finalGLo = vmull_u8(vget_low_u8(pixelsA.val[1]), vget_low_u8(veva));
                        uint16x8_t finalBLo = vmull_u8(vget_low_u8(pixelsA.val[2]), vget_low_u8(veva));

                        finalRLo = vmlal_u8(finalRLo, vget_low_u8(redB), vget_low_u8(vevb));
                        finalGLo = vmlal_u8(finalGLo, vget_low_u8(greenB), vget_low_u8(vevb));
                        finalBLo = vmlal_u8(finalBLo, vget_low_u8(blueB), vget_low_u8(vevb));

                        uint16x8_t finalRHi = vmull_high_u8(pixelsA.val[0], veva);
                        uint16x8_t finalGHi = vmull_high_u8(pixelsA.val[1], veva);
                        uint16x8_t finalBHi = vmull_high_u8(pixelsA.val[2], veva);

                        finalRHi = vmlal_high_u8(finalRHi, redB, vevb);
                        finalGHi = vmlal_high_u8(finalGHi, greenB, vevb);
                        finalBHi = vmlal_high_u8(finalBHi, blueB, vevb);

                        uint8x16_t finalR = vminq_u8(vshrn_high_n_u16(vshrn_n_u16(finalRLo, 4), finalRHi, 4), vdupq_n_u8(0x1F));
                        uint8x16_t finalG = vminq_u8(vshrn_high_n_u16(vshrn_n_u16(finalGLo, 4), finalGHi, 4), vdupq_n_u8(0x1F));
                        uint8x16_t finalB = vminq_u8(vshrn_high_n_u16(vshrn_n_u16(finalBLo, 4), finalBHi, 4), vdupq_n_u8(0x1F));

                        uint8x16_t alpha = vorrq_u8(
                            vandq_u8(evaMask, alphaA),
                            vandq_u8(evbMask, alphaB));

                        uint8x16x2_t finalVal;
                        finalVal.val[0] = vorrq_u8(finalR, vshlq_n_u8(finalG, 5));
                        finalVal.val[1] = vbslq_u8(vdupq_n_u8(0x7F), vorrq_u8(vshlq_n_u8(finalB, 2), vshrq_n_u8(finalG, 3)), alpha);

                        u32 writePos = dstaddr;
                        vst2q_u8((u8*)&dst[dstaddr], finalVal);
                        markCaptureWritten(writePos, 16);

                        dstaddr = (dstaddr + 16) & 0xFFFF;
                    }
                }
            }
            else
            {
                uint8x16_t evaMask = vdupq_n_u8(eva ? 0xFF : 0);
                for (u32 j = 0; j < NativeHeight; j++)
                {
                    u32 dstaddr;
                    if (!getCaptureDstAddr(j, dstaddr))
                        continue;

                    for (u32 i = 0; i < width; i += 16)
                    {
                        uint8x16x4_t pixelsA = vld4q_u8(&srcA[(i + j * 256) * 4]);
                        uint8x16_t alphaA = vtstq_u8(pixelsA.val[3], pixelsA.val[3]);
                        pixelsA.val[0] = vandq_u8(vshrq_n_u8(pixelsA.val[0], 1), alphaA);
                        pixelsA.val[1] = vandq_u8(vshrq_n_u8(pixelsA.val[1], 1), alphaA);
                        pixelsA.val[2] = vandq_u8(vshrq_n_u8(pixelsA.val[2], 1), alphaA);

                        uint8x16_t veva = vdupq_n_u8(eva);

                        uint16x8_t finalRLo = vmull_u8(vget_low_u8(pixelsA.val[0]), vget_low_u8(veva));
                        uint16x8_t finalGLo = vmull_u8(vget_low_u8(pixelsA.val[1]), vget_low_u8(veva));
                        uint16x8_t finalBLo = vmull_u8(vget_low_u8(pixelsA.val[2]), vget_low_u8(veva));

                        uint16x8_t finalRHi = vmull_high_u8(pixelsA.val[0], veva);
                        uint16x8_t finalGHi = vmull_high_u8(pixelsA.val[1], veva);
                        uint16x8_t finalBHi = vmull_high_u8(pixelsA.val[2], veva);

                        uint8x16_t finalR = vshrn_high_n_u16(vshrn_n_u16(finalRLo, 4), finalRHi, 4);
                        uint8x16_t finalG = vshrn_high_n_u16(vshrn_n_u16(finalGLo, 4), finalGHi, 4);
                        uint8x16_t finalB = vshrn_high_n_u16(vshrn_n_u16(finalBLo, 4), finalBHi, 4);

                        uint8x16_t alpha = vandq_u8(evaMask, alphaA);

                        uint8x16x2_t finalVal;
                        finalVal.val[0] = vorrq_u8(finalR, vshlq_n_u8(finalG, 5));
                        finalVal.val[1] = vbslq_u8(vdupq_n_u8(0x7F), vorrq_u8(vshlq_n_u8(finalB, 2), vshrq_n_u8(finalG, 3)), alpha);

                        u32 writePos = dstaddr;
                        vst2q_u8((u8*)&dst[dstaddr], finalVal);
                        markCaptureWritten(writePos, 16);
                        dstaddr = (dstaddr + 16) & 0xFFFF;
                    }
                }
            }
        }
        break;
    }
}

void DekoRenderer::VBlankEnd(Unit* unitA, Unit* unitB)
{
    
}

void DekoRenderer::FillinCurComposeRegion(ComposeRegion& out, bool forceblank)
{
    out.ForceBlank = forceblank;
    out.DispCnt = CurUnit->DispCnt;
    for (int i = 0; i < 4; i++)
        out.BGCnt[i] = CurUnit->BGCnt[i];
    out.BlendCnt = CurUnit->BlendCnt;
    out.MasterBrightness = CurUnit->MasterBrightness;
    out.EVA = CurUnit->EVA;
    out.EVB = CurUnit->EVB;
    out.EVY = CurUnit->EVY;
}

void DekoRenderer::DrawScanline_BGOBJ(u32 line, u32 outputLine)
{
    // forced blank disables BG/OBJ compositing
    if (CurUnit->DispCnt & (1<<7))
    {
        memset(&DirectBitmap[CurUnit->Num][outputLine*256], 0xFF, 256*2);
        DirectBitmapNeeded[CurUnit->Num] = true;

        for (int i = 0; i < 4; i++)
            BGState[CurUnit->Num][i] = bgState_Disable;
        return;
    }

    u32* window = &ComposeUniforms[CurUnit->Num].Window[outputLine*4];

    window[0] = (u32)CurUnit->WinCnt[2] | ((u32)CurUnit->WinCnt[1] << 16);
    window[1] = (u32)CurUnit->WinCnt[0] | ((u32)CurUnit->WinCnt[3] << 16);

    // if all windows are disabled then outside == everything enabled
    if ((CurUnit->DispCnt & 0xE000) == 0)
        window[0] = 0xFF;

    if (CurUnit->DispCnt & (1<<15))
    {
        window[1] |= 0x80000000;
    }

    if (CurUnit->DispCnt & (1<<14) && CurUnit->Win1Active & 1)
    {
        if (CurUnit->Win1Coords[0] > CurUnit->Win1Coords[1])
            window[2] = (u32)CurUnit->Win1Coords[0] | (256 << 16);
        else
            window[2] = (u32)CurUnit->Win1Coords[0] | ((u32)CurUnit->Win1Coords[1] << 16);
    }
    else
    {
        window[2] = 256|(256<<16);
    }

    if (CurUnit->DispCnt & (1<<13) && CurUnit->Win0Active & 1)
    {
        if (CurUnit->Win0Coords[0] > CurUnit->Win0Coords[1])
            window[3] = (u32)CurUnit->Win0Coords[0] | (256 << 16);
        else
            window[3] = (u32)CurUnit->Win0Coords[0] | ((u32)CurUnit->Win0Coords[1] << 16);
    }
    else
    {
        window[3] = 256|(256<<16);
    }

//    ApplySpriteMosaicX();
//    CurBGXMosaicTable = MosaicTable[CurUnit->BGMosaicSize[0]];

    switch (CurUnit->DispCnt & 0x7)
    {
    case 0: DrawScanlineBGMode<0>(line, outputLine); break;
    case 1: DrawScanlineBGMode<1>(line, outputLine); break;
    case 2: DrawScanlineBGMode<2>(line, outputLine); break;
    case 3: DrawScanlineBGMode<3>(line, outputLine); break;
    case 4: DrawScanlineBGMode<4>(line, outputLine); break;
    case 5: DrawScanlineBGMode<5>(line, outputLine); break;
    case 6: DrawScanlineBGMode6(line, outputLine); break;
    case 7: DrawScanlineBGMode7(line, outputLine); break;
    }

    if (CurUnit->BGMosaicY >= CurUnit->BGMosaicYMax)
    {
        CurUnit->BGMosaicY = 0;
        CurUnit->BGMosaicYMax = CurUnit->BGMosaicSize[1];
    }
    else
    {
        CurUnit->BGMosaicY++;
    }
}

template<u32 bgmode>
void DekoRenderer::DrawScanlineBGMode(u32 line, u32 outputLine)
{
    u32 dispCnt = CurUnit->DispCnt;
    if (dispCnt & 0x0800)
    {
        if (bgmode >= 3)
            DrawBG_Extended(outputLine, 3);
        else if (bgmode >= 1)
            DrawBG_Affine(outputLine, 3);
        else
            DrawBG_Text(line, outputLine, 3);
    }
    else
    {
        BGState[CurUnit->Num][3] = bgState_Disable;
    }
    if (dispCnt & 0x0400)
    {
        if (bgmode == 5)
            DrawBG_Extended(outputLine, 2);
        else if (bgmode == 4 || bgmode == 2)
            DrawBG_Affine(outputLine, 2);
        else
            DrawBG_Text(line, outputLine, 2);
    }
    else
    {
        BGState[CurUnit->Num][2] = bgState_Disable;
    }
    if (dispCnt & 0x0200)
    {
        DrawBG_Text(line, outputLine, 1);
    }
    else
    {
        BGState[CurUnit->Num][1] = bgState_Disable;
    }
    if (dispCnt & 0x0100)
    {
        if (!CurUnit->Num && (dispCnt & 0x8))
            DrawBG_3D(outputLine);
        else
            DrawBG_Text(line, outputLine, 0);
    }
    else
    {
        BGState[CurUnit->Num][0] = bgState_Disable;
    }
}

void DekoRenderer::DrawScanlineBGMode6(u32 line, u32 outputLine)
{
    u32 dispCnt = CurUnit->DispCnt;

    BGState[CurUnit->Num][0] = bgState_Disable;
    BGState[CurUnit->Num][1] = bgState_Disable;

    if (dispCnt & 0x0400)
    {
        DrawBG_Large(outputLine);
    }
    else
    {
        BGState[CurUnit->Num][2] = bgState_Disable;
    }

    if (dispCnt & 0x0100 && !CurUnit->Num && dispCnt & 0x8)
    {
        DrawBG_3D(outputLine);
    }
    else
    {
        BGState[CurUnit->Num][0] = bgState_Disable;
    }
}

void DekoRenderer::DrawScanlineBGMode7(u32 line, u32 outputLine)
{
    u32 dispCnt = CurUnit->DispCnt;
    // mode 7 only has text-mode BG0 and BG1

    if (dispCnt & 0x0200)
    {
        DrawBG_Text(line, outputLine, 1);
    }
    else
    {
        BGState[CurUnit->Num][1] = bgState_Disable;
    }

    if (dispCnt & 0x0100)
    {
        if (!CurUnit->Num && (dispCnt & 0x8))
            DrawBG_3D(outputLine);
        else
            DrawBG_Text(line, outputLine, 0);
    }
    else
    {
        BGState[CurUnit->Num][0] = bgState_Disable;
    }

    BGState[CurUnit->Num][2] = bgState_Disable;
    BGState[CurUnit->Num][3] = bgState_Disable;
}

void DekoRenderer::DrawBG_3D(u32 outputLine)
{
    BGState[CurUnit->Num][0] = bgState_3D;

    /*u32* targetLine = &_3DFramebuffer[outputLine * 256];
    for (u32 i = 0; i < 256; i += 16)
    {
        uint8x16x4_t pixels = vld4q_u8((u8*)&_3DLine[i]);

        pixels.val[3] = vandq_u8(vtstq_u8(pixels.val[3], pixels.val[3]), vorrq_u8(pixels.val[3], vdupq_n_u8(0x40)));

        vst4q_u8((u8*)&targetLine[i], pixels);
    }*/
}

void DekoRenderer::DrawBG_Text(u32 line, u32 outputLine, u32 bgnum)
{
    u16 bgcnt = CurUnit->BGCnt[bgnum];

    u16 xoff = CurUnit->BGXPos[bgnum];
    u16 yoff = CurUnit->BGYPos[bgnum] + line;

    if (bgcnt & 0x0040)
    {
        // vertical mosaic
        yoff -= CurUnit->BGMosaicY;
    }

    BGUniform& uniform = BGTextUniforms[CurUnit->Num][bgnum];

    u32 tilemapaddr;
    if (CurUnit->Num)
    {
        uniform.Text.TilesetAddr = ((bgcnt & 0x003C) << 12);
        tilemapaddr = ((bgcnt & 0x1F00) << 3);
    }
    else
    {
        uniform.Text.TilesetAddr = ((CurUnit->DispCnt & 0x07000000) >> 8) + ((bgcnt & 0x003C) << 12);
        tilemapaddr = ((CurUnit->DispCnt & 0x38000000) >> 11) + ((bgcnt & 0x1F00) << 3);
    }

    if (bgcnt & 0x8000)
    {
        tilemapaddr += ((yoff & 0x1F8) << 3);
        if (bgcnt & 0x4000)
            tilemapaddr += ((yoff & 0x100) << 3);
    }
    else
    {
        tilemapaddr += ((yoff & 0xF8) << 3);
    }

    uniform.Text.BGVRAMMask = CurUnit->Num ? 0x1FFFF : 0x7FFFF;
    uniform.Text.WideXMask = (bgcnt & 0x4000) ? 0x100 : 0;

    uniform.Text.MetaMask = (0x1000000 << bgnum) | 0x800000;

    if (bgcnt & 0x0080)
    {
        BGState[CurUnit->Num][bgnum] = bgState_Text8bpp;

        if (CurUnit->DispCnt & 0x40000000)
        {
            u32 extpalslot = ((bgnum<2) && (bgcnt&0x2000)) ? (2+bgnum) : bgnum;
            uniform.Text.MetaMask |= (extpalslot*16 + (paletteMemory_BGExtPalOffset/512)) << 8;
            uniform.Text.ExtpalMask = 0xFFFFFFFF;
        }
        else
        {
            uniform.Text.ExtpalMask = 0;
        }
    }
    else
    {
        BGState[CurUnit->Num][bgnum] = bgState_Text4bpp;
    }

    uniform.PerLineData[outputLine*4+0] = xoff;
    uniform.PerLineData[outputLine*4+1] = yoff & 0x7;
    uniform.PerLineData[outputLine*4+2] = tilemapaddr;
}

void DekoRenderer::DrawBG_Affine(u32 outputLine, u32 bgnum)
{
    u16 bgcnt = CurUnit->BGCnt[bgnum];

    BGUniform& uniform = BGTextUniforms[CurUnit->Num][bgnum];

    switch (bgcnt & 0xC000)
    {
    case 0x0000: uniform.Affine.XMask = 0x07800; uniform.Affine.YShift = 7-3; break;
    case 0x4000: uniform.Affine.XMask = 0x0F800; uniform.Affine.YShift = 8-3; break;
    case 0x8000: uniform.Affine.XMask = 0x1F800; uniform.Affine.YShift = 9-3; break;
    case 0xC000: uniform.Affine.XMask = 0x3F800; uniform.Affine.YShift = 10-3; break;
    }
    uniform.Affine.MetaMask = (0x1000000 << bgnum) | 0x800000;

    if (bgcnt & 0x2000) uniform.Affine.OfxMask = 0;
    else                uniform.Affine.OfxMask = ~(uniform.Affine.XMask | 0x7FF);

    s16 rotA = CurUnit->BGRotA[bgnum-2];
    s16 rotB = CurUnit->BGRotB[bgnum-2];
    s16 rotC = CurUnit->BGRotC[bgnum-2];
    s16 rotD = CurUnit->BGRotD[bgnum-2];

    s32 rotX = CurUnit->BGXRefInternal[bgnum-2];
    s32 rotY = CurUnit->BGYRefInternal[bgnum-2];

    CurUnit->BGXRefInternal[bgnum-2] += rotB;
    CurUnit->BGYRefInternal[bgnum-2] += rotD;

    if (bgcnt & 0x0040)
    {
        // vertical mosaic
        rotX -= (CurUnit->BGMosaicY * rotB);
        rotY -= (CurUnit->BGMosaicY * rotD);
    }

    uniform.PerLineData[outputLine*4+0] = (u32)rotX;
    uniform.PerLineData[outputLine*4+1] = (u32)rotY;
    uniform.PerLineData[outputLine*4+2] = (u32)(s32)rotA;
    uniform.PerLineData[outputLine*4+3] = (u32)(s32)rotC;

    uniform.Affine.BGVRAMMask = CurUnit->Num ? 0x1FFFF : 0x7FFFF;

    if (CurUnit->Num)
    {
        uniform.Affine.TilesetAddr = ((bgcnt & 0x003C) << 12);
        uniform.Affine.TilemapAddr = ((bgcnt & 0x1F00) << 3);
    }
    else
    {
        uniform.Affine.TilesetAddr = ((CurUnit->DispCnt & 0x07000000) >> 8) + ((bgcnt & 0x003C) << 12);
        uniform.Affine.TilemapAddr = ((CurUnit->DispCnt & 0x38000000) >> 11) + ((bgcnt & 0x1F00) << 3);
    }

    BGState[CurUnit->Num][bgnum] = bgState_Affine;
}

void DekoRenderer::DrawBG_Extended(u32 outputLine, u32 bgnum)
{
    u16 bgcnt = CurUnit->BGCnt[bgnum];

    s16 rotA = CurUnit->BGRotA[bgnum-2];
    s16 rotB = CurUnit->BGRotB[bgnum-2];
    s16 rotC = CurUnit->BGRotC[bgnum-2];
    s16 rotD = CurUnit->BGRotD[bgnum-2];

    s32 rotX = CurUnit->BGXRefInternal[bgnum-2];
    s32 rotY = CurUnit->BGYRefInternal[bgnum-2];

    CurUnit->BGXRefInternal[bgnum-2] += rotB;
    CurUnit->BGYRefInternal[bgnum-2] += rotD;

    if (bgcnt & 0x0040)
    {
        // vertical mosaic
        rotX -= (CurUnit->BGMosaicY * rotB);
        rotY -= (CurUnit->BGMosaicY * rotD);
    }

    BGUniform& uniform = BGTextUniforms[CurUnit->Num][bgnum];

    uniform.PerLineData[outputLine*4+0] = (u32)rotX;
    uniform.PerLineData[outputLine*4+1] = (u32)rotY;
    uniform.PerLineData[outputLine*4+2] = (u32)(s32)rotA;
    uniform.PerLineData[outputLine*4+3] = (u32)(s32)rotC;

    uniform.Affine.BGVRAMMask = CurUnit->Num ? 0x1FFFF : 0x7FFFF;

    if (bgcnt & 0x0080)
    {
        switch (bgcnt & 0xC000)
        {
        case 0x0000: uniform.Affine.XMask = 0x07FFF; uniform.Affine.YMask = 0x07FFF; uniform.Affine.YShift = 7; break;
        case 0x4000: uniform.Affine.XMask = 0x0FFFF; uniform.Affine.YMask = 0x0FFFF; uniform.Affine.YShift = 8; break;
        case 0x8000: uniform.Affine.XMask = 0x1FFFF; uniform.Affine.YMask = 0x0FFFF; uniform.Affine.YShift = 9; break;
        case 0xC000: uniform.Affine.XMask = 0x1FFFF; uniform.Affine.YMask = 0x1FFFF; uniform.Affine.YShift = 9; break;
        }
        uniform.Affine.MetaMask = (0x1000000 << bgnum);

        if (bgcnt & 0x2000)
        {
            uniform.Affine.OfxMask = 0;
            uniform.Affine.OfyMask = 0;
        }
        else
        {
            uniform.Affine.OfxMask = ~uniform.Affine.XMask;
            uniform.Affine.OfyMask = ~uniform.Affine.YMask;
        }

        uniform.Affine.TilemapAddr = ((bgcnt & 0x1F00) << 6);

        if (bgcnt & 0x0004)
        {
            BGState[CurUnit->Num][bgnum] = bgState_ExtendedBitmapDirect;
        }
        else
        {
            uniform.Affine.MetaMask |= 0x800000;
            BGState[CurUnit->Num][bgnum] = bgState_ExtendedBitmap8bpp;
        }
    }
    else
    {
        switch (bgcnt & 0xC000)
        {
        case 0x0000: uniform.Affine.XMask = 0x07800; uniform.Affine.YShift = 7-3; break;
        case 0x4000: uniform.Affine.XMask = 0x0F800; uniform.Affine.YShift = 8-3; break;
        case 0x8000: uniform.Affine.XMask = 0x1F800; uniform.Affine.YShift = 9-3; break;
        case 0xC000: uniform.Affine.XMask = 0x3F800; uniform.Affine.YShift = 10-3; break;
        }
        uniform.Affine.MetaMask = (0x1000000 << bgnum) | 0x800000;

        if (bgcnt & 0x2000) uniform.Affine.OfxMask = 0;
        else                uniform.Affine.OfxMask = ~(uniform.Affine.XMask | 0x7FF);

        if (CurUnit->Num)
        {
            uniform.Affine.TilesetAddr = ((bgcnt & 0x003C) << 12);
            uniform.Affine.TilemapAddr = ((bgcnt & 0x1F00) << 3);
        }
        else
        {
            uniform.Affine.TilesetAddr = ((CurUnit->DispCnt & 0x07000000) >> 8) + ((bgcnt & 0x003C) << 12);
            uniform.Affine.TilemapAddr = ((CurUnit->DispCnt & 0x38000000) >> 11) + ((bgcnt & 0x1F00) << 3);
        }

        if (CurUnit->DispCnt & 0x40000000)
        {
            uniform.Affine.MetaMask |= (bgnum*16 + paletteMemory_BGExtPalOffset/512) << 8;
            uniform.Affine.ExtPalMask = 0xFFFFFFFF;
        }
        else
        {
            uniform.Affine.ExtPalMask = 0;
        }

        BGState[CurUnit->Num][bgnum] = bgState_ExtendedMixed;
    }
}

void DekoRenderer::DrawBG_Large(u32 outputLine)
{
    u16 bgcnt = CurUnit->BGCnt[2];
    BGUniform& uniform = BGTextUniforms[CurUnit->Num][2];

    switch (bgcnt & 0xC000)
    {
    case 0x0000: uniform.Affine.XMask = 0x1FFFF; uniform.Affine.YMask = 0x3FFFF; uniform.Affine.YShift = 9; break;
    case 0x4000: uniform.Affine.XMask = 0x3FFFF; uniform.Affine.YMask = 0x1FFFF; uniform.Affine.YShift = 10; break;
    case 0x8000: uniform.Affine.XMask = 0x1FFFF; uniform.Affine.YMask = 0x0FFFF; uniform.Affine.YShift = 9; break;
    case 0xC000: uniform.Affine.XMask = 0x1FFFF; uniform.Affine.YMask = 0x1FFFF; uniform.Affine.YShift = 9; break;
    }
    uniform.Affine.MetaMask = (0x1000000 << 2) | 0x800000;

    if (bgcnt & 0x2000)
    {
        uniform.Affine.OfxMask = 0;
        uniform.Affine.OfyMask = 0;
    }
    else
    {
        uniform.Affine.OfxMask = ~uniform.Affine.XMask;
        uniform.Affine.OfyMask = ~uniform.Affine.YMask;
    }

    uniform.Affine.TilemapAddr = 0;

    s16 rotA = CurUnit->BGRotA[0];
    s16 rotB = CurUnit->BGRotB[0];
    s16 rotC = CurUnit->BGRotC[0];
    s16 rotD = CurUnit->BGRotD[0];

    s32 rotX = CurUnit->BGXRefInternal[0];
    s32 rotY = CurUnit->BGYRefInternal[0];

    CurUnit->BGXRefInternal[0] += rotB;
    CurUnit->BGYRefInternal[0] += rotD;

    if (bgcnt & 0x0040)
    {
        // vertical mosaic
        rotX -= (CurUnit->BGMosaicY * rotB);
        rotY -= (CurUnit->BGMosaicY * rotD);
    }

    uniform.PerLineData[outputLine*4+0] = (u32)rotX;
    uniform.PerLineData[outputLine*4+1] = (u32)rotY;
    uniform.PerLineData[outputLine*4+2] = (u32)(s32)rotA;
    uniform.PerLineData[outputLine*4+3] = (u32)(s32)rotC;

    uniform.Affine.BGVRAMMask = CurUnit->Num ? 0x1FFFF : 0x7FFFF;

    BGState[CurUnit->Num][2] = bgState_ExtendedBitmap8bpp;
}

void DekoRenderer::FlushOBJDraw(u32 curline)
{
    s32 firstLine = OBJBatchFirstLine[CurUnit->Num];
    s32 linesCount = OBJBatchLinesCount[CurUnit->Num];

    if (firstLine == -1)
    {
        //printf("skipping obj\n");
        OBJBatchFirstLine[CurUnit->Num] = curline;
        OBJBatchLinesCount[CurUnit->Num] = 0;
        if (curline < NativeHeight)
            OBJCompositionDirty[CurUnit->Num] = true;
        return;
    }

    if (OBJBatchLinesCount[CurUnit->Num] == 0)
        return;

    u16* oam = (u16*)&OAMShadow[CurUnit->Num ? 0x400 : 0];
    u32 objDispCnt = LastOBJDispCnt[CurUnit->Num];
    u8 objMosaicSizeX = LastOBJMosaicSizeX[CurUnit->Num];
    u8 objMosaicSizeY = LastOBJMosaicSizeY[CurUnit->Num];

    enum
    {
        spriteKind_Regular4bpp,
        spriteKind_Regular8bpp,
        spriteKind_RegularBitmap,
        spriteKind_Affine4bpp,
        spriteKind_Affine8bpp,
        spriteKind_AffineBitmap,
        spriteKind_RegularWindow4bpp,
        spriteKind_RegularWindow8bpp,
        spriteKind_AffineWindow4bpp,
        spriteKind_AffineWindow8bpp,
        spriteKind_Count
    };

    struct SpriteSpec
    {
        s16 X, Y, Width, Height;
        u32 BaseAddr, StrideShift, Meta;
        float Depth;
    };

    int numSprites[spriteKind_Count] = {};
    SpriteSpec sprites[spriteKind_Count][128];
    int numSpritesTotal = 0;
    int numWindowSpritesTotal = 0;
    bool objMosaicFallback = false;
    bool objHasHiResPath = false;

    OBJUniform uniform;
    uniform.VRAMMask = CurUnit->Num ? 0x1FFFF : 0x3FFFF;

    //printf("sprites %d, %d...\n", firstLine, linesCount);
    for (int sprnum = 0; sprnum < 128; sprnum++)
    {
        u16* attrib = &oam[sprnum*4];

        uniform.RotScaleParams[sprnum] = (float)(s16)oam[sprnum*4+3] / 256.f;

        bool isAffine = attrib[0] & 0x0100;

        if ((attrib[0] & 0x0200) && !isAffine)
            continue;

        u32 spritemode = (attrib[0] >> 10) & 0x3;
        u32 tilenum = attrib[2] & 0x03FF;

        const u8 spritewidth[16] =
        {
            8, 16, 8, 8,
            16, 32, 8, 8,
            32, 32, 16, 8,
            64, 64, 32, 8
        };
        const u8 spriteheight[16] =
        {
            8, 8, 16, 8,
            16, 8, 32, 8,
            32, 16, 32, 8,
            64, 32, 64, 8
        };

        u32 sizeparam = (attrib[0] >> 14) | ((attrib[1] & 0xC000) >> 12);
        u32 width = spritewidth[sizeparam];
        u32 height = spriteheight[sizeparam];
        u32 displayWidth = width;
        u32 displayHeight = height;
        bool doubleSizeAffine = isAffine && (attrib[0] & 0x0200);
        if (doubleSizeAffine)
        {
            displayWidth <<= 1;
            displayHeight <<= 1;
        }

        u32 addr, strideShift = 0;
        int spriteKind;
        u32 prio = (attrib[2] >> 10) & 0x3;
        u32 meta = (prio << 29) | 0x400000U;
        float depthKey = ((sprnum | (prio << 7)) / 512.f);
        //printf("depth key %f %d %d\n", depthKey, prio, sprnum);

        if (isAffine)
        {
            if (doubleSizeAffine)
                strideShift |= 0x800000U;

            strideShift |= ((attrib[1] >> 9) & 0x1F) << 24;
        }
        else
        {
            // flip horizontally
            if (attrib[1] & 0x1000)
                strideShift |= 0x40000000;
            if (attrib[1] & 0x2000)
                strideShift |= 0x80000000;
        }

        if (spritemode == 3)
        {
            if (objDispCnt & 0x40)
            {
                if (objDispCnt & 0x20)
                {
                    // 'reserved'
                    // draws nothing

                    continue;
                }
                else
                {
                    addr = tilenum << (7 + ((objDispCnt >> 22) & 0x1));
                    strideShift |= __builtin_ctz(width) + 1;
                }
            }
            else
            {
                if (objDispCnt & 0x20)
                {
                    addr = ((tilenum & 0x01F) << 4) + ((tilenum & 0x3E0) << 7);
                    strideShift |= 9;
                }
                else
                {
                    addr = ((tilenum & 0x00F) << 4) + ((tilenum & 0x3F0) << 7);
                    strideShift |= 8;
                }
            }

            // bitmap sprites are always semitransparent
            meta |= 0x80000000;

            u32 alpha = (attrib[2] & 0xF000) >> 12;
            if (alpha == 0)
                continue;
            alpha++;

            meta |= alpha << 24;

            spriteKind = isAffine ? spriteKind_AffineBitmap : spriteKind_RegularBitmap;
        }
        else
        {
            meta |= 0x800000; // add paletted flag

            if (spritemode == 1)
            {
                // semi transparent
                meta |= 0x80000000;
            }
            else
            {
                meta |= 0x10000000;
            }

            addr = tilenum;
            if (objDispCnt & 0x10)
            {
                addr <<= ((objDispCnt >> 20) & 0x3);
                strideShift |= __builtin_ctz(width) - 3 + ((attrib[0] & 0x2000) ? 1:0);
            }
            else
            {
                strideShift |= 5;
            }

            strideShift += 5;
            addr <<= 5;

            if (attrib[0] & 0x2000)
            {
                if (objDispCnt & 0x80000000)
                    meta |= (((attrib[2] & 0xF000) >> 12) + paletteMemory_OBJExtPalOffset/512) << 8;
                else
                    meta |= 0x100;

                spriteKind = isAffine
                    ? (spritemode == 2 ? spriteKind_AffineWindow8bpp :  spriteKind_Affine8bpp)
                    : (spritemode == 2 ? spriteKind_RegularWindow8bpp : spriteKind_Regular8bpp);
            }
            else
            {
                meta |= ((attrib[2] & 0xF000) >> 8) + 0x100;

                spriteKind = isAffine
                    ? (spritemode == 2 ? spriteKind_AffineWindow4bpp :  spriteKind_Affine4bpp)
                    : (spritemode == 2 ? spriteKind_RegularWindow4bpp : spriteKind_Regular4bpp);
            }
        }
        SpriteSpec& sprite = sprites[spriteKind][numSprites[spriteKind]];

        sprite.X = (s16)((s32)(attrib[1] << 23) >> 23);
        sprite.Y = attrib[0] & 0xFF;
        // not entirely accurate (it doesn't handle sprites coming off on the other side)
        if (sprite.Y >= 192)
            sprite.Y = sprite.Y - 256;

        sprite.Width = spritewidth[sizeparam];
        sprite.Height = spriteheight[sizeparam];
        sprite.BaseAddr = addr;
        sprite.StrideShift = strideShift;
        sprite.Meta = meta;
        sprite.Depth = depthKey;
        //printf("%d %d %x %x %d %d %d %d %d\n", spriteKind, prio, sprite.BaseAddr, sprite.StrideShift, tilenum, sprite.X, sprite.Y, sprite.Width, sprite.Height);

        if (sprite.Y >= firstLine + linesCount)
            continue;
        if (sprite.Y + (s32)displayHeight <= firstLine)
            continue;

        if (sprite.X + (s32)displayWidth <= 0)
            continue;
        if (sprite.X >= 256)
            continue;

        if (spritemode == 2)
        {
            // OBJ window
            numWindowSpritesTotal++;
        }
        if (spritemode != 2 && (attrib[0] & 0x1000)
            && (objMosaicSizeX > 0 || objMosaicSizeY > 0))
            objMosaicFallback = true;
        if (spritemode != 2 && isAffine)
            objHasHiResPath = true;

        numSprites[spriteKind]++;
        numSpritesTotal++;
    }

    EmuCmdBuf.setScissors(0, {DkScissor{0, (u32)firstLine, 256, (u32)linesCount}});

    if (numSpritesTotal > 0)
    {
        EmuCmdBuf.bindVtxAttribState(
        {
            DkVtxAttribState{0, 0, offsetof(SpriteSpec, X), DkVtxAttribSize_4x16, DkVtxAttribType_Sint, 0},
            DkVtxAttribState{0, 0, offsetof(SpriteSpec, BaseAddr), DkVtxAttribSize_3x32, DkVtxAttribType_Uint, 0},
            DkVtxAttribState{0, 0, offsetof(SpriteSpec, Depth), DkVtxAttribSize_1x32, DkVtxAttribType_Float, 0},
        });
        EmuCmdBuf.bindTextures(DkStage_Fragment, 0,
        {
            dkMakeTextureHandle(descriptorOffset_VRAM8 + textureVRAM_AOBJ + CurUnit->Num, 0),
            dkMakeTextureHandle(descriptorOffset_VRAM16 + textureVRAM_AOBJ + CurUnit->Num, 0),
        });

        EmuCmdBuf.bindUniformBuffer(DkStage_Vertex, 0, Gfx::DataHeap->GpuAddr(OBJUniformMemory), OBJUniformSize);
        EmuCmdBuf.bindUniformBuffer(DkStage_Fragment, 0, Gfx::DataHeap->GpuAddr(OBJUniformMemory), OBJUniformSize);

        EmuCmdBuf.pushConstants(Gfx::DataHeap->GpuAddr(OBJUniformMemory), OBJUniformSize, 0, sizeof(OBJUniform), &uniform);
    }

    
    const dk::Shader* vertShaders[spriteKind_Count] =
    {
        &ShaderOBJRegular,
        &ShaderOBJRegular,
        &ShaderOBJRegular,
        &ShaderOBJAffine,
        &ShaderOBJAffine,
        &ShaderOBJAffine,
        &ShaderOBJRegular,
        &ShaderOBJRegular,
        &ShaderOBJAffine,
        &ShaderOBJAffine
    };
    const dk::Shader* fragShaders[spriteKind_Count] =
    {
        &ShaderOBJ4bpp,
        &ShaderOBJ8bpp,
        &ShaderOBJBitmap,
        &ShaderOBJ4bpp,
        &ShaderOBJ8bpp,
        &ShaderOBJBitmap,
        &ShaderOBJWindow4bpp,
        &ShaderOBJWindow8bpp,
        &ShaderOBJWindow4bpp,
        &ShaderOBJWindow8bpp
    };

    BGOBJRedrawn[CurUnit->Num] |= (1 << 4);

    auto drawOBJLayerAtScale = [&](dk::Image& colorTargetImage, dk::Image& depthTargetImage, u32 renderScale)
    {
        u32 width = NativeWidth * renderScale;
        u32 height = NativeHeight * renderScale;
        DkViewport viewport = {0.f, 0.f, (float)width, (float)height, 0.f, 1.f};
        EmuCmdBuf.setViewports(0, {viewport, viewport});
        EmuCmdBuf.setScissors(0, {DkScissor{0, (u32)firstLine * renderScale, width, (u32)linesCount * renderScale}});

        dk::ImageView colorTarget{colorTargetImage};
        dk::ImageView depthTarget{depthTargetImage};
        EmuCmdBuf.bindRenderTargets({&colorTarget}, &depthTarget);
        EmuCmdBuf.clearColor(0, DkColorMask_R, 0);

        if (numSpritesTotal - numWindowSpritesTotal > 0)
        {
            EmuCmdBuf.clearDepthStencil(true, 1.f, 0, 0);

            EmuCmdBuf.bindDepthStencilState(dk::DepthStencilState{}
                .setDepthWriteEnable(true)
                .setDepthTestEnable(true)
                .setDepthCompareOp(DkCompareOp_Less));

            for (int i = 0; i < spriteKind_RegularWindow4bpp; i++)
            {
                if (numSprites[i] > 0)
                {
                    EmuCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {vertShaders[i], fragShaders[i]});

                    DkGpuAddr vertexBuffer = UploadBuf.UploadData(EmuCmdBuf, numSprites[i]*sizeof(SpriteSpec), (u8*)sprites[i]);
                    EmuCmdBuf.bindVtxBuffer(0, vertexBuffer, numSprites[i]*sizeof(SpriteSpec));
                    EmuCmdBuf.bindVtxBufferState({{sizeof(SpriteSpec), 1}});

                    EmuCmdBuf.draw(DkPrimitive_TriangleStrip, 4, numSprites[i], 0, 0);
                }
            }
        }

        EmuCmdBuf.barrier(DkBarrier_Fragments, DkInvalidateFlags_Image);
        EmuCmdBuf.discardDepthStencil();
    };

    drawOBJLayerAtScale(IntermedFramebuffers[fb_Count * CurUnit->Num + fb_OBJ], OBJDepth, 1);
    bool redrawsFullOBJLayer = firstLine == 0 && linesCount == (s32)NativeHeight;
    bool currentLinesHiRes = OBJHiResLinesValid(CurUnit->Num, (u32)firstLine, (u32)linesCount);
    bool canStartHiResOBJ = objHasHiResPath;
    bool canPreserveHiResOBJ = currentLinesHiRes && !redrawsFullOBJLayer;
    bool canUseHiResOBJ = _3DRenderScale > 1 && !objMosaicFallback
        && (canStartHiResOBJ || canPreserveHiResOBJ);
    if (canUseHiResOBJ)
    {
        drawOBJLayerAtScale(IntermedFramebuffersHiRes[fb_Count * CurUnit->Num + fb_OBJ], OBJDepthHiRes, (u32)_3DRenderScale);
        SetOBJHiResLines(CurUnit->Num, (u32)firstLine, (u32)linesCount, true);
    }
    else
    {
        SetOBJHiResLines(CurUnit->Num, (u32)firstLine, (u32)linesCount, false);
    }

    DkViewport nativeViewport = {0.f, 0.f, (float)NativeWidth, (float)NativeHeight, 0.f, 1.f};
    EmuCmdBuf.setViewports(0, {nativeViewport, nativeViewport});
    EmuCmdBuf.setScissors(0, {DkScissor{0, (u32)firstLine, NativeWidth, (u32)linesCount}});

    if (numWindowSpritesTotal > 0 || !OBJWindowEmpty[CurUnit->Num])
    {
        dk::ImageView colorTarget{OBJWindow[CurUnit->Num]};
        EmuCmdBuf.bindRenderTargets({&colorTarget});
        EmuCmdBuf.clearColor(0, DkColorMask_R, 0);

        BGOBJRedrawn[CurUnit->Num] |= (1 << 5);
        OBJWindowEmpty[CurUnit->Num] = false;

        if (numWindowSpritesTotal > 0)
        {
            EmuCmdBuf.bindColorState(dk::ColorState{}
                .setLogicOp(DkLogicOp_Or));

            for (int i = spriteKind_RegularWindow4bpp; i < spriteKind_Count; i++)
            {
                if (numSprites[i] > 0)
                {
                    EmuCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {vertShaders[i], fragShaders[i]});

                    DkGpuAddr vertexBuffer = UploadBuf.UploadData(EmuCmdBuf, numSprites[i]*sizeof(SpriteSpec), (u8*)sprites[i]);
                    EmuCmdBuf.bindVtxBuffer(0, vertexBuffer, numSprites[i]*sizeof(SpriteSpec));
                    EmuCmdBuf.bindVtxBufferState({{sizeof(SpriteSpec), 1}});

                    EmuCmdBuf.draw(DkPrimitive_TriangleStrip, 4, numSprites[i], 0, 0);
                }
            }

            EmuCmdBuf.bindColorState(dk::ColorState{});
        }
        else if (firstLine == 0 && linesCount == (s32)NativeHeight)
        {
            OBJWindowEmpty[CurUnit->Num] = true;
        }

        EmuCmdBuf.barrier(DkBarrier_Fragments, DkInvalidateFlags_Image);
    }

    OBJBatchFirstLine[CurUnit->Num] += OBJBatchLinesCount[CurUnit->Num];
    OBJBatchLinesCount[CurUnit->Num] = 0;
    if ((u32)curline < NativeHeight)
        OBJCompositionDirty[CurUnit->Num] = true;
}

void DekoRenderer::FlushBGDraw(u32 curline, u32 bgmask)
{
    for (int i = 0; i < 4; i++)
    {
        if (bgmask & (1<<i))
        {
            // we're going in
            if (BGBatchFirstLine[CurUnit->Num][i] != -1)
            {
                if (BGBatchLinesCount[CurUnit->Num][i] == 0)
                    bgmask &= ~(1 << i);
            }
            else
            {
                //printf("skipping bg draw %d %d\n", CurUnit->Num, curline);
                bgmask &= ~(1 << i);
                BGBatchFirstLine[CurUnit->Num][i] = curline;
                BGBatchLinesCount[CurUnit->Num][i] = 0;
            }
        }
    }

    if (bgmask == 0)
        return;

    EmuCmdBuf.bindVtxAttribState({});
    EmuCmdBuf.bindTextures(DkStage_Fragment, 0,
    {
        dkMakeTextureHandle(descriptorOffset_VRAM8 + textureVRAM_ABG + CurUnit->Num, 0),
        dkMakeTextureHandle(descriptorOffset_VRAM16 + textureVRAM_ABG + CurUnit->Num, 0),
        dkMakeTextureHandle(descriptorOffset_MosaicTable, 0)
    });
    EmuCmdBuf.bindUniformBuffer(DkStage_Fragment, 0, Gfx::DataHeap->GpuAddr(BGUniformMemory), BGUniformSize);
    EmuCmdBuf.bindDepthStencilState(dk::DepthStencilState{}
        .setDepthWriteEnable(false)
        .setDepthTestEnable(false));

    for (int i = 0; i < 4; i++)
    {
        s32 firstLine = BGBatchFirstLine[CurUnit->Num][i];
        s32 linesCount = BGBatchLinesCount[CurUnit->Num][i];
        if ((1 << i) & bgmask)
        {
            assert(linesCount > 0);
            assert(firstLine != -1);
            int state = BGState[CurUnit->Num][i];
            bool redrawsFullBGLayer = firstLine == 0 && linesCount == (s32)NativeHeight;
            bool bgHasHiResPath = state >= bgState_Affine;
            bool currentLinesHiRes = BGHiResLinesValid(CurUnit->Num, i, (u32)firstLine, (u32)linesCount);
            bool canStartHiResBG = bgHasHiResPath;
            bool canPreserveHiResBG = currentLinesHiRes && !redrawsFullBGLayer;
            bool canUseHiResBG = _3DRenderScale > 1 && (canStartHiResBG || canPreserveHiResBG);
            if (state >= bgState_Text4bpp)
            {
                BGOBJRedrawn[CurUnit->Num] |= (1 << i);

                u32 mosaicLevel = LastBGCnt[CurUnit->Num][i] & 0x0040 ? LastBGMosaicSizeX[CurUnit->Num] : 0;
                u32 batchEndLine = firstLine + linesCount;
                u32 disableAffineYInterpolation =
                    (i >= 2 && AffineChangedMidframe[CurUnit->Num][i-2]) ? (1U << 16) : 0;
                BGTextUniforms[CurUnit->Num][i].Text.MosaicLevel =
                    mosaicLevel | (batchEndLine << 8) | disableAffineYInterpolation;

                dk::Shader* shaders[] =
                {
                    NULL,
                    NULL,
                    ShaderBGText4Bpp,
                    ShaderBGText8Bpp,
                    ShaderBGAffine,
                    ShaderBGExtendedBitmap8pp,
                    ShaderBGExtendedBitmapDirect,
                    ShaderBGExtendedMixed
                };

                auto drawBGAtScale = [&](dk::Image& target, u32 renderScale)
                {
                    u32 width = NativeWidth * renderScale;
                    u32 height = NativeHeight * renderScale;
                    DkViewport viewport = {0.f, 0.f, (float)width, (float)height, 0.f, 1.f};
                    EmuCmdBuf.setViewports(0, {viewport, viewport});

                    dk::ImageView colorTarget{target};
                    EmuCmdBuf.bindRenderTargets({&colorTarget});
                    EmuCmdBuf.setScissors(0, {DkScissor{0, (u32)firstLine * renderScale, width, (u32)linesCount * renderScale}});

                    BGTextUniforms[CurUnit->Num][i].Text.RenderScale = renderScale;

                    //printf("drawing bg range %d %d %d\n", firstLine, linesCount, mosaicLevel);
                    EmuCmdBuf.pushConstants(Gfx::DataHeap->GpuAddr(BGUniformMemory), BGUniformSize,
                        offsetof(BGUniform, Text), sizeof(BGUniform::Text),
                        &BGTextUniforms[CurUnit->Num][i].Text);
                    EmuCmdBuf.pushConstants(Gfx::DataHeap->GpuAddr(BGUniformMemory), BGUniformSize,
                        offsetof(BGUniform, PerLineData) + firstLine*4*4, 4*4*linesCount,
                        &BGTextUniforms[CurUnit->Num][i].PerLineData[firstLine*4]);
                    EmuCmdBuf.bindShaders(DkStageFlag_GraphicsMask,
                    {
                        &ShaderFullscreenQuad,
                        &shaders[state][mosaicLevel > 0]
                    });
                    EmuCmdBuf.draw(DkPrimitive_TriangleStrip, 4, 1, 0, 0);
                };

                drawBGAtScale(IntermedFramebuffers[fb_Count * CurUnit->Num + fb_BG0 + i], 1);

                if (canUseHiResBG)
                {
                    drawBGAtScale(IntermedFramebuffersHiRes[fb_Count * CurUnit->Num + fb_BG0 + i], (u32)_3DRenderScale);
                    SetBGHiResLines(CurUnit->Num, i, (u32)firstLine, (u32)linesCount, true);
                }
                else
                {
                    SetBGHiResLines(CurUnit->Num, i, (u32)firstLine, (u32)linesCount, false);
                }
            }
            else
            {
                BGOBJRedrawn[CurUnit->Num] |= (1 << i);

                auto clearBGAtScale = [&](dk::Image& target, u32 renderScale)
                {
                    u32 width = NativeWidth * renderScale;
                    u32 height = NativeHeight * renderScale;
                    DkViewport viewport = {0.f, 0.f, (float)width, (float)height, 0.f, 1.f};
                    EmuCmdBuf.setViewports(0, {viewport, viewport});

                    dk::ImageView colorTarget{target};
                    EmuCmdBuf.bindRenderTargets({&colorTarget});
                    EmuCmdBuf.setScissors(0, {DkScissor{0, (u32)firstLine * renderScale, width, (u32)linesCount * renderScale}});
                    EmuCmdBuf.clearColor(0, DkColorMask_R, 0);
                };

                clearBGAtScale(IntermedFramebuffers[fb_Count * CurUnit->Num + fb_BG0 + i], 1);
                if (canUseHiResBG)
                {
                    clearBGAtScale(IntermedFramebuffersHiRes[fb_Count * CurUnit->Num + fb_BG0 + i], (u32)_3DRenderScale);
                    SetBGHiResLines(CurUnit->Num, i, (u32)firstLine, (u32)linesCount, true);
                }
                else
                {
                    SetBGHiResLines(CurUnit->Num, i, (u32)firstLine, (u32)linesCount, false);
                }
            }

            assert(BGBatchLinesCount[CurUnit->Num][i] > 0);
            BGBatchFirstLine[CurUnit->Num][i] += BGBatchLinesCount[CurUnit->Num][i];
            BGBatchLinesCount[CurUnit->Num][i] = 0;
        }
    }

    DkViewport nativeViewport = {0.f, 0.f, (float)NativeWidth, (float)NativeHeight, 0.f, 1.f};
    EmuCmdBuf.setViewports(0, {nativeViewport, nativeViewport});

    EmuCmdBuf.barrier(DkBarrier_Tiles, DkInvalidateFlags_Image);
}

void DekoRenderer::ComposeBGOBJ()
{
    dk::ImageView colorTarget{FinalFramebuffers[GPU::FrontBuffer^1][CurUnit->Num == UnitAIsTop]};
    dk::Shader* shader;
    bool capture = CurUnit->Num == 0 && CaptureLatch && !(CaptureCnt & (1<<24));
    EmuCmdBuf.bindVtxAttribState({});

    EmuCmdBuf.bindTextures(DkStage_Fragment, 5,
    {
        dkMakeTextureHandle(descriptorOffset_Palettes + CurUnit->Num, 0),
        dkMakeTextureHandle(descriptorOffset_DirectBitmap + CurUnit->Num, 0),
        dkMakeTextureHandle(descriptorOffset_OBJWindow + CurUnit->Num, 0)
    });

    EmuCmdBuf.bindUniformBuffer(DkStage_Fragment, 0, Gfx::DataHeap->GpuAddr(ComposeUniformMemory), ComposeUniformSize);
    EmuCmdBuf.pushConstants(Gfx::DataHeap->GpuAddr(ComposeUniformMemory), ComposeUniformSize,
        offsetof(ComposeUniform, Window), sizeof(ComposeUniforms[CurUnit->Num].Window),
        &ComposeUniforms[CurUnit->Num].Window);

    //printf("compositing image %d\n", CurUnit->Num);
    u32 firstLine = 0;
    for (ComposeRegion& region : ComposeRegions[CurUnit->Num])
    {
        DkGpuAddr paletteMemory = Gfx::TextureHeap->GpuAddr(PaletteTextureMemory) + paletteMemory_UnitSize * CurUnit->Num;
        if (region.StdPalSize)
        {
            EmuCmdBuf.copyBuffer(
                region.StdPalSrc,
                paletteMemory + region.StdPalDst,
                region.StdPalSize);
        }
        if (region.BGExtPalSize)
        {
            EmuCmdBuf.copyBuffer(
                region.BGExtPalSrc,
                paletteMemory + paletteMemory_BGExtPalOffset + region.BGExtPalDst,
                region.BGExtPalSize);
        }
        if (region.OBJExtPalSize)
        {
            EmuCmdBuf.copyBuffer(
                region.OBJExtPalSrc,
                paletteMemory + paletteMemory_OBJExtPalOffset + region.OBJExtPalDst,
                region.OBJExtPalSize);
        }
        if (region.StdPalSize || region.BGExtPalSize || region.OBJExtPalSize)
        {
            EmuCmdBuf.barrier(DkBarrier_Full, DkInvalidateFlags_Image);
        }

        u32 dispmode = region.DispCnt >> 16;
        dispmode &= (CurUnit->Num ? 0x1 : 0x3);
        bool showDirectBitmap = dispmode != 1 || (region.DispCnt & (1<<7)) || region.ForceBlank;
        bool directBitmapFullWhite = dispmode == 0 || (region.DispCnt & (1<<7)) || region.ForceBlank;

        if (capture)
        {
            shader = showDirectBitmap ? &ShaderComposeBGOBJShowBitmap : &ShaderComposeBGOBJ;
        }
        else
        {
            shader = showDirectBitmap ? &ShaderComposeBGOBJDirectBitmapOnly : &ShaderComposeBGOBJ;
        }
        EmuCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {&ShaderFullscreenQuad, shader});

        u32 outputWidth = NativeWidth * _3DRenderScale;
        u32 outputHeight = NativeHeight * _3DRenderScale;
        DkViewport finalViewport = {0.f, 0.f, (float)outputWidth, (float)outputHeight, 0.f, 1.f};
        EmuCmdBuf.setViewports(0, {finalViewport, finalViewport});
        DkScissor scissor = {0, firstLine * (u32)_3DRenderScale, outputWidth, region.LinesCount * (u32)_3DRenderScale};
        EmuCmdBuf.setScissors(0, {scissor, scissor});
        //printf("compositing region %d %d\n", firstLine, region.LinesCount);

        u32 bgOrder[4];
        int n = 0;
        for (int i = 3; i >= 0; i--)
            for (int j = 3; j >= 0; j--)
                if ((region.BGCnt[j] & 0x3) == i)
                    bgOrder[n++] = j;

        ComposeUniform& composeUniform = ComposeUniforms[CurUnit->Num];
        int textureHandleIdx[4];
        int captureTextureHandleIdx[4];
        composeUniform.HiResBGMask = 0;
        composeUniform.HiResOBJ = 0;

        for (int i = 0; i < 4; i++)
        {
            int curPrio = region.BGCnt[bgOrder[i]] & 0x3;
            int nextPrio = i < 3 ? (region.BGCnt[bgOrder[i + 1]] & 0x3) : -1;

            if (i == 0)
            {
                // insert all the sprites which come before the bg with the lowest priority
                composeUniform.BGSpriteMasks[0] = 0;
                for (int i = 3; i > curPrio; i--)
                    composeUniform.BGSpriteMasks[0] |= 1 << i;
            }

            // sprites are inserted wherever the priority of two backgrounds differs
            composeUniform.BGSpriteMasks[i+1] = 0;

            for (int j = curPrio; j > nextPrio; j--)
                composeUniform.BGSpriteMasks[i+1] |= 1 << j;

            if (!(region.DispCnt & (1<<(bgOrder[i]+8))))
            {
                textureHandleIdx[i] = descriptorOffset_DisabledBG;
                captureTextureHandleIdx[i] = descriptorOffset_DisabledBG;
            }
            else if (bgOrder[i] == 0 && CurUnit->Num == 0 && region.DispCnt & (1<<3))
            {
                textureHandleIdx[i] = _3DRenderScale > 1
                    ? descriptorOffset_3DFramebufferHiRes
                    : descriptorOffset_3DFramebuffer;
                captureTextureHandleIdx[i] = descriptorOffset_3DFramebuffer;
                if (_3DRenderScale > 1)
                    composeUniform.HiResBGMask |= 1U << i;
            }
            else
            {
                int nativeTextureHandleIdx = descriptorOffset_IntermedFb +
                    fb_BG0 + fb_Count * CurUnit->Num + bgOrder[i];
                captureTextureHandleIdx[i] = nativeTextureHandleIdx;

                if (_3DRenderScale > 1 && BGHiResLinesValid(CurUnit->Num, bgOrder[i], firstLine, region.LinesCount))
                {
                    textureHandleIdx[i] = descriptorOffset_IntermedFbHiRes +
                        fb_BG0 + fb_Count * CurUnit->Num + bgOrder[i];
                    composeUniform.HiResBGMask |= 1U << i;
                }
                else
                {
                    textureHandleIdx[i] = nativeTextureHandleIdx;
                }
            }

            composeUniform.BGNumMask[i] = 1 << bgOrder[i];
        }

        int nativeObjTextureHandleIdx = region.DispCnt & (1<<12)
            ? (descriptorOffset_IntermedFb + fb_OBJ + fb_Count * CurUnit->Num)
            : descriptorOffset_DisabledBG;
        int objTextureHandleIdx = nativeObjTextureHandleIdx;
        if (_3DRenderScale > 1 && (region.DispCnt & (1<<12))
            && OBJHiResLinesValid(CurUnit->Num, firstLine, region.LinesCount))
        {
            objTextureHandleIdx = descriptorOffset_IntermedFbHiRes + fb_OBJ + fb_Count * CurUnit->Num;
            composeUniform.HiResOBJ = 1;
        }

        assert(n == 4);
        EmuCmdBuf.bindTextures(DkStage_Fragment, 0,
        {
            dkMakeTextureHandle(textureHandleIdx[0], 0),
            dkMakeTextureHandle(textureHandleIdx[1], 0),
            dkMakeTextureHandle(textureHandleIdx[2], 0),
            dkMakeTextureHandle(textureHandleIdx[3], 0),
            dkMakeTextureHandle(objTextureHandleIdx, 0),
        });

        composeUniform.MasterBrightnessFactor = region.MasterBrightness & 0x1F;
        composeUniform.MasterBrightnessMode = region.MasterBrightness >> 14;
        if (composeUniform.MasterBrightnessFactor == 0)
            composeUniform.MasterBrightnessMode = 0;
        if (composeUniform.MasterBrightnessFactor > 16)
            composeUniform.MasterBrightnessFactor = 16;
        if (dispmode == 0 || region.ForceBlank)
            composeUniform.MasterBrightnessMode = 0;
        composeUniform.BlendCnt = region.BlendCnt;
        composeUniform.StandardColorEffect = (region.BlendCnt >> 6) & 0x3;
        composeUniform.EVA = region.EVA;
        composeUniform.EVB = region.EVB;
        composeUniform.EVY = region.EVY;
        composeUniform.RenderScale = _3DRenderScale;
        composeUniform.FinalScale = _3DRenderScale;
        composeUniform.DirectBitmapFullWhite = directBitmapFullWhite ? 1 : 0;
        EmuCmdBuf.pushConstants(Gfx::DataHeap->GpuAddr(ComposeUniformMemory), ComposeUniformSize,
            0, sizeof(ComposeUniform)-sizeof(composeUniform.Window),
            &composeUniform);

        EmuCmdBuf.bindRenderTargets({&colorTarget});
        EmuCmdBuf.draw(DkPrimitive_TriangleStrip, 4, 1, 0, 0);

        if (capture)
        {
            dk::Shader* captureShader = ((region.DispCnt & (1<<7)) || region.ForceBlank)
                ? &ShaderComposeBGOBJShowBitmap
                : &ShaderComposeBGOBJ;
            EmuCmdBuf.bindShaders(DkStageFlag_GraphicsMask, {&ShaderFullscreenQuad, captureShader});

            DkViewport nativeViewport = {0.f, 0.f, (float)NativeWidth, (float)NativeHeight, 0.f, 1.f};
            EmuCmdBuf.setViewports(0, {nativeViewport, nativeViewport});
            DkScissor nativeScissor = {0, firstLine, NativeWidth, region.LinesCount};
            EmuCmdBuf.setScissors(0, {nativeScissor, nativeScissor});

            EmuCmdBuf.bindTextures(DkStage_Fragment, 0,
            {
                dkMakeTextureHandle(captureTextureHandleIdx[0], 0),
                dkMakeTextureHandle(captureTextureHandleIdx[1], 0),
                dkMakeTextureHandle(captureTextureHandleIdx[2], 0),
                dkMakeTextureHandle(captureTextureHandleIdx[3], 0),
                dkMakeTextureHandle(nativeObjTextureHandleIdx, 0),
            });

            composeUniform.RenderScale = 1;
            composeUniform.FinalScale = 1;
            composeUniform.HiResBGMask = 0;
            composeUniform.HiResOBJ = 0;
            composeUniform.DirectBitmapFullWhite = ((region.DispCnt & (1<<7)) || region.ForceBlank) ? 1 : 0;
            EmuCmdBuf.pushConstants(Gfx::DataHeap->GpuAddr(ComposeUniformMemory), ComposeUniformSize,
                0, sizeof(ComposeUniform)-sizeof(composeUniform.Window),
                &composeUniform);

            dk::ImageView captureColorTarget{DisplayCaptureColorTarget};
            dk::ImageView bgobj{BGOBJTexture};
            EmuCmdBuf.bindRenderTargets({&captureColorTarget, &bgobj});
            EmuCmdBuf.draw(DkPrimitive_TriangleStrip, 4, 1, 0, 0);
        }

        firstLine += region.LinesCount;
    }
    assert(firstLine == 192);

    DkViewport nativeViewport = {0.f, 0.f, (float)NativeWidth, (float)NativeHeight, 0.f, 1.f};
    EmuCmdBuf.setViewports(0, {nativeViewport, nativeViewport});
    DkScissor nativeScissor = {0, 0, NativeWidth, NativeHeight};
    EmuCmdBuf.setScissors(0, {nativeScissor, nativeScissor});

    ComposeRegions[CurUnit->Num].clear();

    EmuCmdBuf.barrier(DkBarrier_Fragments, DkInvalidateFlags_Image);

    for (int i = 0; i < 5; i++)
    {
        if (BGOBJRedrawn[CurUnit->Num] & (1 << i))
        {
            dk::ImageView colorTarget{IntermedFramebuffers[fb_Count * CurUnit->Num + fb_BG0 + i]};
            EmuCmdBuf.bindRenderTargets({&colorTarget});
            EmuCmdBuf.discardColor(0);

            if (i < 4 && (BGHiResValid[CurUnit->Num] & (1U << i)))
            {
                dk::ImageView hiResColorTarget{IntermedFramebuffersHiRes[fb_Count * CurUnit->Num + fb_BG0 + i]};
                EmuCmdBuf.bindRenderTargets({&hiResColorTarget});
                EmuCmdBuf.discardColor(0);
            }
            else if (i == 4 && OBJHiResValid[CurUnit->Num])
            {
                dk::ImageView hiResColorTarget{IntermedFramebuffersHiRes[fb_Count * CurUnit->Num + fb_OBJ]};
                EmuCmdBuf.bindRenderTargets({&hiResColorTarget});
                EmuCmdBuf.discardColor(0);
            }
        }
    }
    if (BGOBJRedrawn[CurUnit->Num] & (1 << 5))
    {
        dk::ImageView colorTarget{OBJWindow[CurUnit->Num]};
        EmuCmdBuf.bindRenderTargets({&colorTarget});
        EmuCmdBuf.discardColor(0);
    }

    BGOBJRedrawn[CurUnit->Num] = 0;

    if (CurUnit->Num == 0 && CaptureLatch)
    {
        dk::ImageView src{CaptureCnt & (1<<24) ? _3DFramebuffer : BGOBJTexture};
        EmuCmdBuf.copyImageToBuffer(src, {0, 0, 0, 256, 192, 1}, {Gfx::DataHeap->GpuAddr(DisplayCaptureMemory)});
        EmuCmdBuf.signalFence(DisplayCaptureFence, true);
    }
}

}
