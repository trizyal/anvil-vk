// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UIELEMENTS_H
#define ANVIL_VK_UIELEMENTS_H

/**
 * @file UIElements.h
 * @brief Free functions for different UI elements.
 */

#include <glm/glm.hpp>

namespace UI
{
    void FrameStats();

    /**
     * @brief Renders a debug 3D orientation axis overlay in a corner of the viewport.
     * @param viewMatrix Current active camera view matrix used to orient the widget's axes.
     */
    void RenderWorldAxes(const glm::mat4& viewMatrix);
}



#endif //ANVIL_VK_UIELEMENTS_H
