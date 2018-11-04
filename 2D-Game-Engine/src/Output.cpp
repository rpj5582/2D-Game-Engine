#include "stdafx.h"
#include "Output.h"

#include <iostream>

void Output::log(std::string message)
{
#ifdef _DEBUG
	std::cout << message << std::endl;
#endif
}

void Output::error(std::string message)
{
#ifdef _DEBUG
	std::cout << message << std::endl;
	__debugbreak();
#endif
}
