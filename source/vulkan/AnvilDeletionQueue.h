#ifndef ANVIL_VK_DELETIONQUEUE_H
#define ANVIL_VK_DELETIONQUEUE_H

#include <deque>
#include <functional>

struct AnvilDeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push(std::function<void()>&& function)
    {
        deletors.push_back(function);
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
