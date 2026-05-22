#ifndef ANVIL_VK_FILEIO_H
#define ANVIL_VK_FILEIO_H

#include <string>
#include <vector>

namespace AnvilFileIO
{
    std::vector<char> ReadFile(const std::string &filename);
};

#endif //ANVIL_VK_FILEIO_H
