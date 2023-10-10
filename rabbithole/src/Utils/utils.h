#pragma once

#include "common.h"

#include <string>
#include <vector>
#include <filesystem>

namespace Utils
{
	long long ProfileSetStartTime();
	void ProfileSetEndTimeAndPrint(long long start, std::string label = "Print time");
	// returns profile time in microsec
	long long ProfileGetEndTime(long long start);

	void PrintMatrix(const Matrix44f& matrix);

	std::vector<char> ReadFile(const std::string& filepath);
	std::filesystem::path FindResFolder(const std::filesystem::path& currentDir = std::filesystem::current_path());
	
	void* RabbitMalloc(size_t size);
	void RabbitFree(void* ptr);
}
