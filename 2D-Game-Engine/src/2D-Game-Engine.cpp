#include "stdafx.h"

#include "Window.h"

int main()
{
	// Memory leak detection
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Window window(640, 480, "Title");
	window.mainLoop();

    return 0;
}

