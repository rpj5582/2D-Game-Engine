#include "stdafx.h"
#include "Output.h"

#include <iostream>

void Output::log(std::string message)
{
	std::cout << message << std::endl;
}

void Output::error(std::string message)
{
	std::cout << message << std::endl;
	__debugbreak();
}
