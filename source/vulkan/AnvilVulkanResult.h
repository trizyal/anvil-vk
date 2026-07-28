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
    // Helper function to format and throw the error
    inline void CheckVulkanResult(VkResult result, const char* functionName, const char* file, int line)
    {
        if (result != VK_SUCCESS)
        {
            std::string errorMsg = "Vulkan Error [" + std::to_string(result) + "]\n" +
                                   "File: " + file + ":" + std::to_string(line) + "\n" +
                                   "Call: " + functionName + "\n";

            // Optional: You can also std::cerr << errorMsg << '\n'; here if you want it in the console
            throw std::runtime_error(errorMsg);
        }
    }
}

// The macro that captures the code text (#x), the file name, and the line number
#define CHECK(x) AnvilResult::CheckVulkanResult((x), #x, __FILE__, __LINE__)

#endif //ANVIL_VK_VULKANRESULT_H
