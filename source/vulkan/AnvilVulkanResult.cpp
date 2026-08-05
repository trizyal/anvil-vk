// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilVulkanResult.h"

#include <sstream>

namespace AnvilResult
{
	void CheckVulkanResult(const VkResult aResult, const char* functionName, const char* file, const int line)
    {
    	if (aResult != VK_SUCCESS)
    	{
    		const std::string error_message = "Vulkan Error [" + ToString(aResult) + "]\n" +
								   "File: " + file + ":" + std::to_string(line) + "\n" +
								   "Call: " + functionName + "\n";

    		// Optional: Can also use std::cerr here if we want it in the console.
    		throw std::runtime_error(error_message);
    	}
    }

	std::string ToString(const VkResult aResult)
    {
		// See:
		// https://docs.vulkan.org/refpages/latest/refpages/source/VkResult.html
		switch(aResult)
		{
#			define CASE_(x) case VK_##x: return #x
			CASE_(SUCCESS);
			CASE_(NOT_READY);
			CASE_(TIMEOUT);
			CASE_(EVENT_SET);
			CASE_(EVENT_RESET);
			CASE_(INCOMPLETE);
			CASE_(ERROR_OUT_OF_HOST_MEMORY);
			CASE_(ERROR_OUT_OF_DEVICE_MEMORY);
			CASE_(ERROR_INITIALIZATION_FAILED);
			CASE_(ERROR_DEVICE_LOST);
			CASE_(ERROR_MEMORY_MAP_FAILED);
			CASE_(ERROR_LAYER_NOT_PRESENT);
			CASE_(ERROR_EXTENSION_NOT_PRESENT);
			CASE_(ERROR_FEATURE_NOT_PRESENT);
			CASE_(ERROR_INCOMPATIBLE_DRIVER);
			CASE_(ERROR_TOO_MANY_OBJECTS);
			CASE_(ERROR_FORMAT_NOT_SUPPORTED);
			CASE_(ERROR_FRAGMENTED_POOL);
			CASE_(ERROR_UNKNOWN);
			CASE_(ERROR_OUT_OF_POOL_MEMORY);
			CASE_(ERROR_INVALID_EXTERNAL_HANDLE);
			CASE_(ERROR_FRAGMENTATION);
			CASE_(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
			CASE_(ERROR_SURFACE_LOST_KHR);
			CASE_(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			CASE_(SUBOPTIMAL_KHR);
			CASE_(ERROR_OUT_OF_DATE_KHR);
			CASE_(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			CASE_(ERROR_VALIDATION_FAILED_EXT);
			CASE_(ERROR_INVALID_SHADER_NV);
			CASE_(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
			CASE_(ERROR_NOT_PERMITTED_EXT);
			CASE_(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
			CASE_(THREAD_IDLE_KHR);
			CASE_(THREAD_DONE_KHR);
			CASE_(OPERATION_DEFERRED_KHR);
			CASE_(OPERATION_NOT_DEFERRED_KHR);
			CASE_(PIPELINE_COMPILE_REQUIRED_EXT);
			CASE_(ERROR_COMPRESSION_EXHAUSTED_EXT);

			CASE_(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
			CASE_(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
			CASE_(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
			CASE_(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
			CASE_(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
			CASE_(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);

			// New ones that weren't here before.
			CASE_(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR);
			CASE_(INCOMPATIBLE_SHADER_BINARY_EXT);
			CASE_(PIPELINE_BINARY_MISSING_KHR);
			CASE_(ERROR_NOT_ENOUGH_SPACE_KHR);

			CASE_(ERROR_PRESENT_TIMING_QUEUE_FULL_EXT);
#			undef CASE_

			// Vulkan includes this extra value in the enumeration. We should never
			// see it in practice - it's handled by the fallback option at the end.
			// If this case wasn't included here, most compilers will emit a
			// warning on unhandled cases in the switch().
			case VK_RESULT_MAX_ENUM: break;

			// Most compilers will warn if any enumeration values were missed in
			// the switch(). This is nice, as it will tell us if new VkResult
			// values are added to Vulkan. However, if this isn't desirable,
			// uncomment the following line:
			//default: break;
		}

		// Handle other values gracefully.
		std::ostringstream oss;
		oss << "VkResult(" << std::underlying_type_t<VkResult>(aResult) << ")";
		return oss.str();
	}
}
