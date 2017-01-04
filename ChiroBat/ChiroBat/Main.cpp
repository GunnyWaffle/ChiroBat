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

	// problems:
	// 1) over-stepping responsibilities between functions
	// 2) no "don't grow" option

#define TYPE int

	size_t dataCount = 13;
	TYPE* data = (TYPE*)MEMORY.malloc(sizeof(TYPE) * dataCount);
	TYPE* data2 = (TYPE*)MEMORY.calloc(sizeof(TYPE) * dataCount);
	
	for (TYPE i = 0; i < dataCount; ++i)
	{
		data[i] = i;
		data2[i] += dataCount + i;
	}
	
	for (TYPE i = 0; i < dataCount; ++i)
		printf("%d: %d\n", i, data[i]);

	for (TYPE i = 0; i < dataCount; ++i)
		printf("%d: %d\n", i, data2[i]);
	
	MEMORY.free(data);
	MEMORY.free(data2);

	engineState = ENGINE.shutDown();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to shutdown");

	return EXIT_SUCCESS;
}
