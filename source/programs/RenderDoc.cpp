// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderDoc.h"

#include <cassert>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

RENDERDOC_API_1_1_2* RenderDoc::rdoc_api = nullptr;

void RenderDoc::InitializeRenderDoc()
{
    std::cout << "Initializing RenderDoc." << std::endl;
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = nullptr;

#ifdef _WIN32
    // Loads the dll if RenderDoc is installed and in the system PATH, or if launched via RenderDoc
    if (HMODULE mod = LoadLibraryA("renderdoc.dll"))
    {
        RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
    }
    else
    {
        DWORD error = GetLastError();

        LPSTR message = nullptr;

        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            0,
            reinterpret_cast<LPSTR>(&message),
            0,
            nullptr
        );

        std::cerr << "LoadLibraryA failed (" << error << "): "
                  << (message ? message : "Unknown error")
                  << '\n';

        LocalFree(message);
    }
#else
    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW))
    {
        RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
    }
#endif // _WIN32

    if (RENDERDOC_GetAPI)
    {
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&rdoc_api));
        assert(ret == 1);

        std::cout << "[RenderDoc] API loaded successfully." << std::endl;

        // Hide RenderDoc onscreen UI
        rdoc_api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
    }
}

void RenderDoc::TriggerCapture()
{
    if (rdoc_api)
    {
        rdoc_api->TriggerCapture();

        // If the UI isn't already open, launch it automatically and connect
        if (!rdoc_api->IsTargetControlConnected())
        {
            rdoc_api->LaunchReplayUI(1, "");
        }
    }
    else
    {
        std::cerr << "[RenderDoc] Cannot capture: API not initialized or renderdoc.dll missing." << std::endl;
    }
}
