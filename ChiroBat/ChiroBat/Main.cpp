#include "engine\Engine.h"
#include "engine\Debug.h"
#include "engine\Memory.h"

using namespace ChiroBat;

int main()
{
	funcRet engineState;

	engineState = ENGINE.init();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to initialize");

	printf("beep boop\n");

	engineState = ENGINE.shutDown();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to shutdown");

	return EXIT_SUCCESS;
}
