// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANRESULT_H
#define ANVIL_VK_VULKANRESULT_H

#include <stdexcept>
#include <string>
#include <iostream>

#include <volk.h>

namespace AnvilResult
{
    std::string ToString(VkResult aResult);

    // Helper function to format and throw the error
    void CheckVulkanResult(const VkResult aResult, const char* functionName, const char* file, const int line);
} //AnvilResult

// The macro that captures the code text (#x), the file name, and the line number
#define CHECK(x) AnvilResult::CheckVulkanResult((x), #x, __FILE__, __LINE__)

#endif //ANVIL_VK_VULKANRESULT_H
