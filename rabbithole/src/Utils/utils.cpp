#include "utils.h"

#include <iostream>
#include <chrono>
#include <ctime>

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
}

