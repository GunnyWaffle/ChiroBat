#include <malloc.h>

#include "Memory.h"
#include "Debug.h"

namespace ChiroBat
{
	namespace Memory
	{
		funcRet MemoryManager::init()
		{
			RET_ON_ERR(pool, EXIT_FAILURE, "[Memory Manager] re-allocation of the pool was attempted");

			pool = (byte*)_mm_malloc(MEMORY_POOL_SIZE, alignof(void*));
			RET_ON_ERR(!pool, EXIT_FAILURE, "[Memory Manager] failed to allocate memory pool");

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::shutDown()
		{
			_mm_free(pool);
			return EXIT_SUCCESS;
		}
	}
}