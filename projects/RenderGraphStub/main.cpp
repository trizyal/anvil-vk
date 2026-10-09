// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderGraphStub.h"

int main()
{
    RenderGraphApp app;

    app.initialize();
    app.run();
    app.cleanup();

    return 0;
}