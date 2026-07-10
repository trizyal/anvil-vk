// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_DELETIONQUEUE_H
#define ANVIL_VK_DELETIONQUEUE_H

#include <deque>
#include <functional>

struct AnvilDeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()>&& function)
    {
        deletors.push_back(std::move(function));
    }

    void flush()
    {
        // Reverse iteration
        // destroy in the opposite order to creation
        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
        {
            (*it)();
        }
        deletors.clear();
    }
};

#endif //ANVIL_VK_DELETIONQUEUE_H
