// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FRUSTUM_H
#define FRUSTUM_H
#include "Trace.h"

/**
 * @file Frustum.h
 */

/**
 * @brief Axis-Aligned Bounding Box used for spatial partitioning and frustum culling.
 */
struct AABB
{
    /** Minimum bounds of the box. Initialized to float max to allow expansion. */
    glm::vec3 min = glm::vec3(FLT_MAX);

    /** Maximum bounds of the box. Initialized to float min to allow expansion. */
    glm::vec3 max = glm::vec3(-FLT_MAX);

    /**
     * @brief Calculates the exact center point of the bounding box.
     * @return The 3D center coordinate.
     */
    [[nodiscard]] glm::vec3 getCenter() const
    {
        return (min + max) * 0.5f;
    }

    /**
     * @brief Calculates the half-extents (distance from center to edge) along all three axes.
     * @return The 3D extents vector.
     */
    [[nodiscard]] glm::vec3 getExtents() const
    {
        return (max - min) * 0.5f;
    }
};

/**
 * @brief Represents a 3D camera viewing frustum defined by six bounding planes.
 */
struct Frustum
{
    /** The 6 planes of the frustum: Left, Right, Top, Bottom, Near, Far. */
    glm::vec4 planes[6];

    /**
     * @brief Extracts normalized frustum planes from a view-projection matrix using the Gribb/Hart method.
     * @param vp The combined View-Projection matrix of the active camera.
     * @note Specifically accounts for Vulkan's inverted Y-axis coordinate system.
     */
    void extractPlanes(const glm::mat4& vp)
    {
        SCOPE_CPU;

        for (int i = 0; i < 4; ++i) planes[0][i] = vp[i][3] + vp[i][0]; // Left
        for (int i = 0; i < 4; ++i) planes[1][i] = vp[i][3] - vp[i][0]; // Right
        for (int i = 0; i < 4; ++i) planes[2][i] = vp[i][3] - vp[i][1]; // Top (Vulkan inverted Y)
        for (int i = 0; i < 4; ++i) planes[3][i] = vp[i][3] + vp[i][1]; // Bottom
        for (int i = 0; i < 4; ++i) planes[4][i] = /*vp[i][3] +*/ vp[i][2]; // Near
        for (int i = 0; i < 4; ++i) planes[5][i] = vp[i][3] - vp[i][2]; // Far

        for (glm::vec4& plane : planes)
        {
            const float length = glm::length(glm::vec3(plane));
            plane /= length;
        }
    }

    /**
     * @brief Tests if an Axis-Aligned Bounding Box intersects or sits fully inside the frustum.
     *
     * @param aabb The bounding box to test against the frustum planes.
     * @return True if the AABB is visible, false if it is completely outside (culled).
     */
    bool contains(const AABB& aabb)
    {
        SCOPE_CPU;
        
        const glm::vec3 center = aabb.getCenter();
        const glm::vec3 extents = aabb.getExtents();

        for (const glm::vec4& plane : planes)
        {
            const float effective_radius = extents.x * std::abs(plane.x)
                + extents.y * std::abs(plane.y)
                + extents.z * std::abs(plane.z);

            const float signed_distance = glm::dot(glm::vec3(plane), center) + plane.w;
            if (signed_distance < -effective_radius)
            {
                return false; // outside the frustum
            }
        }
        return true;
    }
};

#endif //FRUSTUM_H
