#include "Shader.h"
#include "Logger/Logger.h"

#include <fstream>

std::vector<char> Shader::ReadShaderFromFile(std::string filepath)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
    {
        LOG_CRITICAL("File doesnt exists!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
