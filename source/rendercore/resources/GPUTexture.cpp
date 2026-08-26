// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUTexture.h"

/*
GPUTexture::GPUTexture(GPUTexture&& other) noexcept
{
    *this = std::move(other);
}

GPUTexture& GPUTexture::operator=(GPUTexture&& other) noexcept
{
    if (this != &other)
    {
        if (pContext) destroyTexture(pContext);

        image = other.image;
        allocation = other.allocation;
        imageView = other.imageView;
        sampler = other.sampler;
        pContext = other.pContext;

        other.image = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
        other.imageView = VK_NULL_HANDLE;
        other.sampler = VK_NULL_HANDLE;
    }
    return *this;
}
*/
