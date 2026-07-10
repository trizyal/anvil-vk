// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_FILEIO_H
#define ANVIL_VK_FILEIO_H

#include <string>
#include <vector>

namespace AnvilFileIO
{
    std::vector<char> ReadFile(const std::string &filename);
};

#endif //ANVIL_VK_FILEIO_H
