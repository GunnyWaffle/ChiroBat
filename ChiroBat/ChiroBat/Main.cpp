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

	//for (size_t i = 0; i < sizeof(size_t) * 8; ++i)
	//{
	//	size_t a = (size_t)1 << i;
	//	printf("b %zu: MSB %d: LSB %d\n", i, MEMORY.findMSB(a), MEMORY.findLSB(a));
	//}

	size_t intCount = 5;
	int* ints = (int*)MEMORY.malloc(sizeof(int) * intCount);
	int* ints2 = (int*)MEMORY.calloc(sizeof(int) * intCount);
	
	for (int i = 0; i < intCount; ++i)
	{
		ints[i] = i;
		ints2[i] += intCount + i;
	}
	
	for (int i = 0; i < intCount; ++i)
		printf("%d: %d\n", i, ints[i]);

	for (int i = 0; i < intCount; ++i)
		printf("%d: %d\n", i, ints2[i]);
	
	MEMORY.free(ints);
	MEMORY.free(ints2);

	engineState = ENGINE.shutDown();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to shutdown");

	return EXIT_SUCCESS;
}
