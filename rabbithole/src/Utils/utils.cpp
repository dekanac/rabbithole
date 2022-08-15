#include "utils.h"

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

}

