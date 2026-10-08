// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file Index32.h
 */

#include <compare>
#include "Ensure.h"


/**
 * @brief Zero-overhead strongly-typed wrapper for array and vector indices.
 *
 * Designed to eliminate `-Wsign-conversion` warnings across the codebase while
 * providing strict runtime bounds checking. It defaults to `-1` (invalid) and
 * safely bridges the gap between signed arithmetic and unsigned `size_t` container bounds.
 */
struct Index32
{
    /** @brief The raw 32-bit signed index. -1 denotes an unassigned or invalid index. */
    int value = -1;

    /**
     * @brief Default constructor. Initializes the index to -1 (invalid).
     */
    Index32() = default;

    /**
     * @brief Implicitly constructs an index from a standard signed integer.
     * @param v The raw integer value (e.g., 5, -1, 0).
     */
    /* implicit */ Index32(int v) : value(v) {}

    /**
     * @brief Implicitly downcasts a 64-bit unsigned size into a 32-bit signed index.
     * @param v A size_t value (usually from `std::vector::size()`).
     * @note Triggers a fatal assertion if the size exceeds the 32-bit signed integer limit.
     */
    /* implicit */ Index32(size_t v) : value(static_cast<int>(v))
    {
        ENSURE(v <= 2147483647, "Vector size exceeds 32-bit integer limits!");
    }

    /**
     * @brief Implicit conversion to `size_t` for direct use inside container brackets (e.g., `vec[idx]`).
     * @return The unsigned 64-bit representation of the index.
     * @note Triggers a fatal assertion if the index is negative at the time of access.
     */
    /* implicit */ operator size_t() const
    {
        ENSURE(value >= 0, "Attempted to index a vector with a negative value!");
        return static_cast<size_t>(value);
    }

    // ----------------------------------------------------
    // Loop Iterator Support
    // ----------------------------------------------------

    /** @brief Pre-increment operator (++i). */
    Index32& operator++()
    {
        ++value;
        return *this;
    }

    /** @brief Post-increment operator (i++). */
    Index32 operator++(int)
    {
        Index32 temp = *this;
        ++value;
        return temp;
    }

    /** @brief Pre-decrement operator (--i). */
    Index32& operator--()
    {
        --value;
        return *this;
    }

    /** @brief Post-decrement operator (i--). */
    Index32 operator--(int)
    {
        Index32 temp = *this;
        --value;
        return temp;
    }

    // ----------------------------------------------------
    // Safe Comparisons
    // ----------------------------------------------------

    /** @brief Default C++20 spaceship operator for comparing two Index32 objects. */
    auto operator<=>(const Index32&) const = default;

    /** @brief Equality check against a raw signed integer. */
    bool operator==(int rhs) const
    {
        return value == rhs;
    }

    /** @brief Three-way comparison against a raw signed integer (handles <, >, <=, >=). */
    auto operator<=>(int rhs) const
    {
        return value <=> rhs;
    }

    /**
     * @brief Safely checks equality against a 64-bit unsigned size.
     * @param rhs A size_t bound (e.g., `vec.size()`).
     * @return False if the index is negative, otherwise performs an exact match.
     */
    bool operator==(size_t rhs) const
    {
        return value >= 0 && static_cast<size_t>(value) == rhs;
    }

    /**
     * @brief Safely performs relational comparisons (<, >) against a 64-bit unsigned size.
     * @param rhs A size_t bound (e.g., `vec.size()`).
     * @return std::strong_ordering result of the comparison.
     * @note Negative indices are safely evaluated as strictly less than any unsigned size_t.
     */
    auto operator<=>(size_t rhs) const
    {
        if (value < 0)
        {
            return std::strong_ordering::less;
        }

        return static_cast<size_t>(value) <=> rhs;
    }

    /**
     * @brief Safely checks equality against a 32-bit unsigned integer.
     */
    bool operator==(uint32_t rhs) const
    {
        return value >= 0 && static_cast<uint32_t>(value) == rhs;
    }

    /**
     * @brief Safely performs relational comparisons (<, >) against a 32-bit unsigned integer.
     */
    auto operator<=>(uint32_t rhs) const
    {
        if (value < 0)
        {
            return std::strong_ordering::less;
        }

        return static_cast<uint32_t>(value) <=> rhs;
    }
};
