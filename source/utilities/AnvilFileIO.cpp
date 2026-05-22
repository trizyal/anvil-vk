#include "AnvilFileIO.h"

#include <fstream>
#include <stdexcept>

namespace AnvilFileIO
{
    std::vector<char> readFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ifstream::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file " + filename);
        }

        const size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
}
