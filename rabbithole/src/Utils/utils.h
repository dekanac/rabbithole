#pragma once

#include "common.h"

#include <string>
#include <vector>

namespace Utils
{
	long long SetStartTime();
	void SetEndtimeAndPrint(long long start);

	void PrintMatrix(const rabbitMat4f& matrix);
	std::vector<char> ReadFile(const std::string& filepath);
}
