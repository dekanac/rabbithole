#include "utils.h"

#include "Logger/Logger.h"

#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace Utils
{
	long long ProfileSetStartTime()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	void ProfileSetEndTimeAndPrint(long long start, std::string label)
	{
		auto endtime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::cout << label << ": " << endtime - start << " us" << std::endl;
	}
	
	long long ProfileGetEndTime(long long start)
	{
		auto endtime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		return endtime - start;
	}

	void PrintMatrix(const Matrix44f& matrix)
	{
		std::cout << std::setprecision(2) << std::fixed;
		for (int i = 0; i < matrix.length(); i++)
		{
			for (int j = 0; j < matrix[0].length(); j++)
			{
				std::cout << matrix[i][j] << "|";
			}
			std::cout << '\n';
		}
		std::cout << '\n';
	}

	std::vector<char> ReadFile(const std::string& filepath)
	{
		std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

		if (!file.is_open())
		{
			LOG_ERROR("failed to open file: " + filepath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	void* RabbitMalloc(size_t size)
	{
		void* allocatedData = malloc(size);
		ASSERT(allocatedData != nullptr, "Allocation failed!");
		return allocatedData;
	}

	void RabbitFree(void* ptr)
	{
		ASSERT(ptr != nullptr, "Free failed!");
		free(ptr);
	}

	// Function to search for the "res" folder recursively in parent directories
	std::filesystem::path FindResFolder(const std::filesystem::path& currentDir) 
	{
		std::filesystem::path resDir = currentDir / "res";

		// Check if the "res" folder exists in the current directory
		if (std::filesystem::exists(resDir) && std::filesystem::is_directory(resDir)) {
			return resDir;
		}

		// If not found, check parent directory
		std::filesystem::path parentDir = currentDir.parent_path();
		if (!parentDir.empty()) {
			return FindResFolder(parentDir);
		}

		// If we reach the root directory and still haven't found "res," return an empty path
		return "";
	}

}

