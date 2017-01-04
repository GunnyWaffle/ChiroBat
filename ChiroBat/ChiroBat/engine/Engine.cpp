#include "Engine.h"
#include "Debug.h"
#include "Memory.h"

namespace ChiroBat
{
	namespace Engine
	{
		funcRet Engine::init()
		{
			funcRet systemState;
			
			systemState = MEMORY.init(1024 - 1);
			RET_ON_ERR(systemState, EXIT_FAILURE, "[Engine] Memory failed to initialize");

			return EXIT_SUCCESS;
		}

		funcRet Engine::shutDown()
		{
			funcRet systemState;
			
			systemState = MEMORY.shutDown();
			RET_ON_ERR(systemState, EXIT_FAILURE, "[Engine] Memory failed to shutdown");
			
			return EXIT_SUCCESS;
		}
	}
}