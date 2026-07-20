// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilShaders.h"

#include <fstream>
#include <iostream>

namespace AnvilShaders
{
SlangStage ConvertToSlangStage(ShaderType inShaderType)
{
    switch (inShaderType)
    {
    case ST_Vertex:
        return SLANG_STAGE_VERTEX;

    case ST_Fragment:
        return SLANG_STAGE_FRAGMENT;

    case ST_Compute:
        return SLANG_STAGE_COMPUTE;

    default:
        return SLANG_STAGE_NONE;
    }
}

void DumpSPIRVToFile(std::vector<uint32_t> inSPIRV, std::string& filename)
{
    // Open in binary mode!
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        // Cast the uint32_t array to a char array so file.write can process it bytes-wise
        file.write(reinterpret_cast<const char*>(inSPIRV.data()), inSPIRV.size() * sizeof(uint32_t));
        file.close();
        std::cout << "Saved SPIR-V dump to: " << filename << "\n";
    } else {
        std::cerr << "Failed to open file for SPIR-V dump: " << filename << "\n";
    }
}

} //AnvilShaders
