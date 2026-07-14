/*
    Copyright 2016-2021 Arisotura

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include <cstring>
#include "Savestate.h"
#include "Platform.h"

/*
    Savestate format

    header:
    00 - magic MELN
    04 - version major
    06 - version minor
    08 - length
    0C - reserved (should be game serial later!)

    section header:
    00 - section magic
    04 - section length
    08 - reserved
    0C - reserved

    Implementation details

    version difference:
    * different major means savestate file is incompatible
    * different minor means adjustments may have to be made
*/


const u32 QUICKSAVE_DEFAULT_SIZE = 32 * 1024 * 1024; // 32 MB
const u32 SAVESTATE_FILE_BUFFER_SIZE = 1024 * 1024; // 1 MB
u8 quicksave_buf[QUICKSAVE_DEFAULT_SIZE] = {0};  // static memory buffer

static bool IsMemorySavestate(const char* filename)
{
    size_t len = strlen(filename);
    return len >= 3 && strncmp(filename + len - 3, "mem", 3) == 0;
}

void Savestate::SetFileBuffer()
{
    if (!file)
        return;

    FileBuffer = new u8[SAVESTATE_FILE_BUFFER_SIZE];
    setvbuf(file, (char*)FileBuffer, _IOFBF, SAVESTATE_FILE_BUFFER_SIZE);
}


Savestate::Savestate(const char* filename, bool save)
{
    const char* magic = "MELN";

    file = nullptr;
    FileBuffer = nullptr;
    Error = false;
    Finished = false;
    StateLength = 0;

    if (save)
    {
        Saving = true;

        if (IsMemorySavestate(filename))
            file = fmemopen(quicksave_buf, QUICKSAVE_DEFAULT_SIZE, "wb");
        else
        {
            file = Platform::OpenLocalFile(filename, "wb");
            SetFileBuffer();
        }
        
        if (!file)
        {
            printf("savestate: file %s doesn't exist\n", filename);
            Error = true;
            return;
        }

        VersionMajor = SAVESTATE_MAJOR;
        VersionMinor = SAVESTATE_MINOR;

        fwrite(magic, 4, 1, file);
        fwrite(&VersionMajor, 2, 1, file);
        fwrite(&VersionMinor, 2, 1, file);
        fseek(file, 8, SEEK_CUR); // length to be fixed later
    }
    else
    {
        Saving = false;
        
        if (IsMemorySavestate(filename))
            file = fmemopen(quicksave_buf, QUICKSAVE_DEFAULT_SIZE, "rb");
        else
        {
            file = Platform::OpenFile(filename, "rb");
            SetFileBuffer();
        }
            
        if (!file)
        {
            printf("savestate: file %s doesn't exist\n", filename);
            Error = true;
            return;
        }

        u32 len;
        fseek(file, 0, SEEK_END);
        len = (u32)ftell(file);
        fseek(file, 0, SEEK_SET);

        u32 buf = 0;

        fread(&buf, 4, 1, file);
        if (buf != ((u32*)magic)[0])
        {
            printf("savestate: invalid magic %08X\n", buf);
            Error = true;
            return;
        }

        VersionMajor = 0;
        VersionMinor = 0;

        fread(&VersionMajor, 2, 1, file);
        if (VersionMajor != SAVESTATE_MAJOR)
        {
            printf("savestate: bad version major %d, expecting %d\n", VersionMajor, SAVESTATE_MAJOR);
            Error = true;
            return;
        }

        fread(&VersionMinor, 2, 1, file);
        if (VersionMinor > SAVESTATE_MINOR)
        {
            printf("savestate: state from the future, %d > %d\n", VersionMinor, SAVESTATE_MINOR);
            Error = true;
            return;
        }

        buf = 0;
        fread(&buf, 4, 1, file);
        StateLength = buf;
        /*
        if (buf != len)
        {
            printf("savestate: bad length %d\n", buf);
            Error = true;
            return;
        }*/

        fseek(file, 4, SEEK_CUR);
    }

    CurSection = -1;
}

Savestate::~Savestate()
{
    Finish();

    if (file) fclose(file);
    delete[] FileBuffer;
}

void Savestate::Finish()
{
    if (Finished || !file || Error || !Saving)
        return;

    if (CurSection != 0xFFFFFFFF)
    {
        u32 pos = (u32)ftell(file);
        fseek(file, CurSection+4, SEEK_SET);

        u32 len = pos - CurSection;
        fwrite(&len, 4, 1, file);

        fseek(file, pos, SEEK_SET);
        CurSection = 0xFFFFFFFF;
    }

    fseek(file, 0, SEEK_END);
    u32 len = (u32)ftell(file);
    fseek(file, 8, SEEK_SET);
    fwrite(&len, 4, 1, file);
    fseek(file, len, SEEK_SET);

    StateLength = len;
    Finished = true;
}

bool Savestate::SeekAfterState()
{
    if (Error || Saving || !file || StateLength == 0)
        return false;

    return fseek(file, StateLength, SEEK_SET) == 0;
}

void Savestate::Section(const char* magic)
{
    if (Error) return;

    if (Saving)
    {
        if (CurSection != 0xFFFFFFFF)
        {
            u32 pos = (u32)ftell(file);
            fseek(file, CurSection+4, SEEK_SET);

            u32 len = pos - CurSection;
            fwrite(&len, 4, 1, file);

            fseek(file, pos, SEEK_SET);
        }

        CurSection = (u32)ftell(file);

        fwrite(magic, 4, 1, file);
        fseek(file, 12, SEEK_CUR);
    }
    else
    {
        fseek(file, 0x10, SEEK_SET);

        for (;;)
        {
            u32 buf = 0;

            fread(&buf, 4, 1, file);
            if (buf != ((u32*)magic)[0])
            {
                if (buf == 0)
                {
                    printf("savestate: section %s not found. blarg\n", magic);
                    return;
                }

                buf = 0;
                fread(&buf, 4, 1, file);
                fseek(file, buf-8, SEEK_CUR);
                continue;
            }

            fseek(file, 12, SEEK_CUR);
            break;
        }
    }
}

void Savestate::Var8(u8* var)
{
    if (Error) return;

    if (Saving)
    {
        fwrite(var, 1, 1, file);
    }
    else
    {
        fread(var, 1, 1, file);
    }
}

void Savestate::Var16(u16* var)
{
    if (Error) return;

    if (Saving)
    {
        fwrite(var, 2, 1, file);
    }
    else
    {
        fread(var, 2, 1, file);
    }
}

void Savestate::Var32(u32* var)
{
    if (Error) return;

    if (Saving)
    {
        fwrite(var, 4, 1, file);
    }
    else
    {
        fread(var, 4, 1, file);
    }
}

void Savestate::Var64(u64* var)
{
    if (Error) return;

    if (Saving)
    {
        fwrite(var, 8, 1, file);
    }
    else
    {
        fread(var, 8, 1, file);
    }
}

void Savestate::Bool32(bool* var)
{
    // for compability
    if (Saving)
    {
        u32 val = *var;
        Var32(&val);
    }
    else
    {
        u32 val;
        Var32(&val);
        *var = val != 0;
    }
}

void Savestate::VarArray(void* data, u32 len)
{
    if (Error) return;

    if (Saving)
    {
        fwrite(data, len, 1, file);
    }
    else
    {
        fread(data, len, 1, file);
    }
}
