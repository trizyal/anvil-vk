#ifndef ANVIL_VK_FILEIO_H
#define ANVIL_VK_FILEIO_H

#include <string>
#include <vector>

namespace AnvilFileIO
{
    static std::vector<char> readFile(const std::string &filename);
};

#endif //ANVIL_VK_FILEIO_H
