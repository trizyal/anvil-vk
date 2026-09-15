// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_DEBUGMODES_H
#define ANVIL_VK_DEBUGMODES_H

#ifdef __cplusplus
    #include <cstdint>
    #define CREATE_ENUM(Name) enum class Name : uint32_t
#else
    #define CREATE_ENUM(Name) enum Name
#endif

/**
 * @brief Render view modes supported by the debug pass.
 */
CREATE_ENUM(DebugMode)
{
    None            = 0,

    BaseColor       = 1,
    GeometryNormal  = 2,
    RawNormalMap    = 3,
    WorldNormal     = 4,
    Metallic        = 5,
    Roughness       = 6,
    Depth           = 7,
    Overdraw        = 8,
    Overshading     = 9,

    Count           = 10
};

#endif //ANVIL_VK_DEBUGMODES_H
