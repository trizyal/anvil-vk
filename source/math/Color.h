// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file Color.h
 */

#include <array>
#include <glm/glm.hpp>
#include <imgui.h>
#include <volk.h>
#include <cstdint>

/**
 * @brief Universal engine color structure with seamless implicit conversions to GLM, ImGui, and Vulkan types.
 */
struct Color
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr Color() = default;
    /* implicit */ constexpr Color(float _r, float _g, float _b, float _a) :
        r(_r), g(_g), b(_b), a(_a) {}

    /** @brief Convert from glm 4-vector. */
    /* implicit */ constexpr Color(const glm::vec4& v) :
        r(v.r), g(v.g), b(v.b), a(v.a) {}

    /** @brief Convert from glm 3-vector. */
    /* implicit */ constexpr Color(const glm::vec3& v) :
        r(v.r), g(v.g), b(v.b) {}

    /** @brief Convert from imgui vector. */
    /* implicit */ constexpr Color(const ImVec4& v) :
        r(v.x), g(v.y), b(v.z), a(v.w) {}

    /** @brief Convert from a float. RGB take the value, A stays as 1.0. */
    explicit constexpr Color(const float& v) :
        r(v), g(v), b(v) {}

    /** @brief Construct from 0-255 scale integers. */
    constexpr Color(int _r, int _g, int _b, int _a = 255)
        : r(static_cast<float>(_r) / 255.0f),
          g(static_cast<float>(_g) / 255.0f),
          b(static_cast<float>(_b) / 255.0f),
          a(static_cast<float>(_a) / 255.0f) {}

    /** @brief Constuct from raw 8-bit bytes (0, 255). */
    /* implicit */ constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255)
        : r(static_cast<float>(_r) / 255.0f),
          g(static_cast<float>(_g) / 255.0f),
          b(static_cast<float>(_b) / 255.0f),
          a(static_cast<float>(_a) / 255.0f) {}

    // SAFE: Compiler enforces an array of exactly 4 elements.
    /** @brief Constuct from raw 8-bit bytes (0, 255) in a 4-element array. */
    /* implicit */ constexpr Color(const std::array<uint8_t, 4>& v)
        : r(static_cast<float>(v[0]) / 255.0f),
          g(static_cast<float>(v[1]) / 255.0f),
          b(static_cast<float>(v[2]) / 255.0f),
          a(static_cast<float>(v[3]) / 255.0f) {}

    // SAFE: Compiler enforces an array of exactly 3 elements.
    /** @brief Constuct from raw 8-bit bytes (0, 255) in a 3-element array. */
    /* implicit */ constexpr Color(const std::array<uint8_t, 3>& v)
        : r(static_cast<float>(v[0]) / 255.0f),
          g(static_cast<float>(v[1]) / 255.0f),
          b(static_cast<float>(v[2]) / 255.0f) {}

    /**
     * @brief Omit alpha value and get rgb.
     * @return rgb values in a glm::vec3.
     */
    [[nodiscard]]
    constexpr glm::vec3 rgb() const
    {
        return glm::vec3{r, g, b};
    }

    /**
     * @brief Packs the normalized floats into a contiguous 4-byte array (R, G, B, A).
     * @return A 4-element array of uint8_t.
     */
    [[nodiscard]]
    constexpr std::array<uint8_t, 4> u8() const
    {
        return{
            static_cast<uint8_t>(r * 255.0f),
            static_cast<uint8_t>(g * 255.0f),
            static_cast<uint8_t>(b * 255.0f),
            static_cast<uint8_t>(a * 255.0f),
        };
    }

    /** @brief Auto-converts to glm::vec4 when needed. */
    /* implicit */ constexpr operator glm::vec4() const
    {
        return glm::vec4{r, g, b, a};
    }

    /** @brief Auto-converts to ImVec4 when needed. */
    /* implicit */ constexpr operator ImVec4() const
    {
        return ImVec4{r, g, b, a};
    }

    /** @brief Auto-converts to ImU32 when needed. */
    /* implicit */ constexpr operator ImU32() const
    {
        return (static_cast<ImU32>(a * 255.0f) << IM_COL32_A_SHIFT) |
               (static_cast<ImU32>(b * 255.0f) << IM_COL32_B_SHIFT) |
               (static_cast<ImU32>(g * 255.0f) << IM_COL32_G_SHIFT) |
               (static_cast<ImU32>(r * 255.0f) << IM_COL32_R_SHIFT);
    }
};

static_assert(sizeof(Color) == 16, "Color struct must be exactly 16 bytes (4x 32-bit floats)");

/**
 * @brief Standardized global palette.
 */
namespace UI
{
#   define COLOR inline constexpr Color

    COLOR White = {1.0f, 1.0f, 1.0f, 1.0f};
    COLOR Black = {0.0f, 0.0f, 0.0f, 1.0f};

    COLOR Gray      = {0.5f, 0.5f, 0.5f, 1.0f};
    COLOR GrayLight = {0.7f, 0.7f, 0.7f, 1.0f};
    COLOR GrayDark  = {0.2f, 0.2f, 0.2f, 1.0f};

    COLOR BgGray        = {0.7f, 0.7f, 0.7f, 0.4f};
    COLOR BgGrayDark    = {0.1f, 0.1f, 0.1f, 0.5f};

    COLOR Red   = {1.0f, 0.0f, 0.0f, 1.0f};
    COLOR Green = {0.0f, 1.0f, 0.0f, 1.0f};
    COLOR Blue  = {0.0f, 0.0f, 1.0f, 1.0f};

    COLOR Yellow    = {1.0f, 0.0f, 0.0f, 1.0f};
    COLOR Cyan      = {0.0f, 1.0f, 1.0f, 1.0f};
    COLOR Magenta   = {1.0f, 0.0f, 1.0f, 1.0f};

    COLOR X = Red;
    COLOR Y = Green;
    COLOR Z = Blue;

    COLOR Transparent = {0.0f, 0.0f, 0.0f, 0.0f};
    COLOR Normal = {128, 128, 255, 255};

#   undef COLOR
}
