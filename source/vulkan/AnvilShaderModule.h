// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERMODULE_H
#define ANVIL_VK_SHADERMODULE_H

#include <volk.h>

#include "AnvilShaders.h"

class AnvilShaderModule
{
private:
    VkDevice anvilDevice = VK_NULL_HANDLE;
    VkShaderModule anvilShaderModule = VK_NULL_HANDLE;

public:
    bool createShaderModule(VkDevice inDevice, const AnvilShaders::ShaderByteCode& inSPIRV);
    void destroyShaderModule() const;

    [[nodiscard]] VkShaderModule get() const;
};


#endif //ANVIL_VK_SHADERMODULE_H
