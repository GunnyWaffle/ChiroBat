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
			
			systemState = Memory::MemoryManager::instance().init();
			RET_ON_ERR(systemState, EXIT_FAILURE, "[Engine] Memory failed to initialize");

			return EXIT_SUCCESS;
		}

		funcRet Engine::shutDown()
		{
			funcRet systemState;
			
			systemState = Memory::MemoryManager::instance().shutDown();
			RET_ON_ERR(systemState, EXIT_FAILURE, "[Engine] Memory failed to initialize");
			
			return EXIT_SUCCESS;
		}
	}
}