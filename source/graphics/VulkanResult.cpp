// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "VulkanResult.h"

#include <sstream>

#include "Ensure.h"
#include "Logger.h"
#include "VulkanStrings.h"

namespace VulkanResult
{
    void CheckVulkanResult(const VkResult aResult, const char* functionName, const char* file, const int line)
    {
        if (aResult != VK_SUCCESS)
        {
            const std::string error_message = "Vulkan Error [" + vk_str(aResult) + "]\n" +
               "File: " + file + ":" + std::to_string(line) + "\n" +
               "Call: " + functionName + "\n";

            LOG_FATAL("{}", error_message);
            FATAL(false, "Vulkan Error");
        }
    }

    void CheckVkBootstrapResult(const std::string& errorMessage, const char* functionName, const char* file, int line)
    {
        const std::string error_message = "vk-bootstrap Error: " + errorMessage + "\n" +
            "File: " + file + ":" + std::to_string(line) + "\n" +
            "Variable: " + functionName + "\n";

        LOG_FATAL("{}", errorMessage);
        FATAL(false, "vk-bootstrap Error");
    }
}
