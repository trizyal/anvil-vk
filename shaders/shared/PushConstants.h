// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_PUSHCONSTANTS_H
#define ANVIL_VK_PUSHCONSTANTS_H

#include "DebugModes.h"

#ifdef __cplusplus
    #include <glm/glm.hpp>
    #include <index32.h>
    #define MAT4 glm::mat4
    #define VEC4 glm::vec4
    #define INDEX tml::index32
#else
    #define MAT4 float4x4
    #define VEC4 float4
    #define INDEX uint32_t
#endif

/**
 * @brief Standardized push constants used globally by the Engine and Projects.
 *
 * Ensures Vulkan validation layers do not throw errors when the engine intercepts
 * project pipelines to inject debug rendering.
 */
struct PushConstants
{
    MAT4 viewProjection;
    VEC4 cameraPosition;
    INDEX objectIndex;
    DebugMode debugMode;
};

#endif //ANVIL_VK_PUSHCONSTANTS_H
