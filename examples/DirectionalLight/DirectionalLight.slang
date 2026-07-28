// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 projectionViewMatrix;
    float4x4 modelMatrix;
}

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]]
ConstantBuffer<PushConstants> pushConstants;

// [[vk::binding(0, 0)]] forces it to use Set 0, Binding 0 to match your C++ setup.
[[vk::binding(0, 0)]]
Sampler2D texSampler;
