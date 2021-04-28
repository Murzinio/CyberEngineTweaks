#include <stdafx.h>

#include "Image.h"

struct PdbInfo
{
    DWORD     Signature;
    BYTE      Guid[16];
    DWORD     Age;
    char      PdbFileName[1];
};

struct ImageVersion
{
    uint8_t Guid[16];
    uint64_t Version;
};

void Image::Initialize()
{
    const ImageVersion cVersions[] = {
        {{0x2B, 0x4E, 0x65, 0x3D, 0xD4, 0x68, 0xC7, 0x42, 0xBF, 0xC9, 0x58, 0xDC, 0x38, 0xD4, 0x2A, 0x36}, MakeVersion(1, 4)},
        {{0x93, 0x5B, 0x36, 0x35, 0xDF, 0xA8, 0xE7, 0x41, 0x91, 0x8A, 0x64, 0x64, 0xF7, 0xA4, 0xF0, 0x8E}, MakeVersion(1, 5)},
        {{0x67, 0xFB, 0x96, 0x6B, 0xAA, 0x3D, 0x57, 0x4E, 0x93, 0x8F, 0x1C, 0xC5, 0x85, 0xc6, 0xF5, 0x29}, MakeVersion(1, 6)},
        {{0xE0, 0xC2, 0x94, 0x64, 0x5C, 0xB4, 0x32, 0x45, 0x95, 0x10, 0x09, 0xA1, 0x0F, 0xB2, 0x53, 0xF8}, MakeVersion(1, 10)},
        {{0x8C, 0x13, 0x59, 0xA9, 0x7E, 0x6E, 0x49, 0x4F, 0x82, 0xF9, 0xCF, 0x58, 0x71, 0x6B, 0x7D, 0x3A}, MakeVersion(1, 11)},
        {{0x7B, 0x51, 0xF5, 0x2C, 0x87, 0xD0, 0xFF, 0x40, 0x83, 0xE5, 0xAA, 0x6C, 0x07, 0xE9, 0x95, 0x20}, MakeVersion(1, 12)},
        {{0x17, 0x1B, 0x2C, 0x4A, 0xC7, 0xA5, 0x38, 0x49, 0x8C, 0x43, 0x20, 0xFC, 0x47, 0x14, 0xF2, 0x33}, MakeVersion(1, 20)},
        {{0xEA, 0xD7, 0xE8, 0xCB, 0x7F, 0x9D, 0x6C, 0x4F, 0x9E, 0x3E, 0x99, 0x13, 0x92, 0x8F, 0x46, 0x62}, MakeVersion(1, 21)},
        {{0xD0, 0x43, 0x98, 0xA6, 0x91, 0xBD, 0xC3, 0x40, 0x85, 0x97, 0xC7, 0xC1, 0x1A, 0xC9, 0x50, 0x91}, MakeVersion(1, 22)},
    };

    mem::module mainModule = mem::module::main();

    auto* pImage = GetModuleHandle(nullptr);
    
    auto sectionHeaders = mainModule.section_headers();

    base_address = reinterpret_cast<uintptr_t>(pImage);

    for (const auto& cHeader : sectionHeaders)
    {
        if (memcmp(cHeader.Name, ".text", 5) == 0)
        {
            TextRegion = mem::region(reinterpret_cast<uint8_t*>(base_address + cHeader.VirtualAddress),
                                  cHeader.Misc.VirtualSize);
            break;
        }
    }

    auto& dosHeader = mainModule.dos_header();
    auto* pFileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(base_address + dosHeader.e_lfanew + 4);
    auto* pOptionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER*>(((char*)pFileHeader) + sizeof(IMAGE_FILE_HEADER));
    auto* pDataDirectory = &pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    auto* pDebugDirectory = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(base_address + pDataDirectory->VirtualAddress);

    // Check to see that the data has the right type
    if (IMAGE_DEBUG_TYPE_CODEVIEW == pDebugDirectory->Type)
    {
        PdbInfo* pdb_info = (PdbInfo*)(base_address + pDebugDirectory->AddressOfRawData);
        if (0 == memcmp(&pdb_info->Signature, "RSDS", 4))
        {
            for (const auto& v : cVersions)
            {
                if (memcmp(&pdb_info->Guid, v.Guid, 16) == 0)
                {
                    version = v.Version;
                    break;
                }
            }

            if (version == 0)
            {
                for (auto c : pdb_info->Guid)
                    spdlog::error("{:X}", (uint32_t)c);
            }
        }
    }
}
