#pragma once

#include <string>

namespace Output
{
#if defined(_DEBUG)
	void log(std::string message);
	void error(std::string message);
#endif
}