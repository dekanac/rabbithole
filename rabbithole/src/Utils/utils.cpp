#include "utils.h"

#include "Logger/Logger.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace Utils
{
	long long SetStartTime()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	void SetEndtimeAndPrint(long long start)
	{
		auto endtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::cout << endtime - start << "ms" << std::endl;
	}

	void PrintMatrix(const rabbitMat4f& matrix)
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

}

