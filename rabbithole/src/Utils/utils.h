#pragma once

#include "common.h"

#include <string>
#include <vector>

namespace Utils
{
	long long ProfileSetStartTime();
	void ProfileSetEndTimeAndPrint(long long start, std::string label = "Print time");
	// returns profile time in microsec
	long long ProfileGetEndTime(long long start);

	void PrintMatrix(const rabbitMat4f& matrix);
	std::vector<char> ReadFile(const std::string& filepath);

	void* RabbitMalloc(size_t size);
	void RabbitFree(void* ptr);
}
