#include "engine\Engine.h"
#include "engine\Debug.h"

using namespace ChiroBat;

int main()
{
	funcRet engineState;

	engineState = Engine::Engine::instance().init();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to initialize");

	printf("beep boop\n");

	engineState = Engine::Engine::instance().shutDown();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to shutdown");

	return EXIT_SUCCESS;
}
