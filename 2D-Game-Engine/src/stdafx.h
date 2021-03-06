// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "Output.h"

namespace std
{
	template<>
	struct hash<glm::vec2>
	{
		size_t operator()(const glm::vec2& vec2) const
		{
			return ((int)vec2.x * 73856093) ^ ((int)vec2.y * 83492791);
		}
	};
}