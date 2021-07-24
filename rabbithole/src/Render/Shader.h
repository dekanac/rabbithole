#pragma once

#include <vector>
#include <string>

class Shader {

public:
    static std::vector<char> ReadShaderFromFile(std::string path);

private:
    Shader();

private:
};