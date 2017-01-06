#include "engine\Engine.h"
#include "engine\Debug.h"
#include "engine\Memory.h"

using namespace ChiroBat;

int main()
{
	funcRet engineState;

	engineState = ENGINE.init();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to initialize");

	//for (size_t i = 0; i < sizeof(size_t) * 8; ++i)
	//{
	//	int b = MEMORY.findMSB((size_t)1 << i);
	//
	//	if (b != i)
	//		printf("bit-%zu MSB-%d\n", i, b);
	//}

	int* data = (int*)MEMORY.malloc(sizeof(int) * 14);
	char* data2 = (char*)MEMORY.calloc(sizeof(char) * 41);
	
	if (data)
		for (int i = 0; i < 14; ++i)
			data[i] = i;

	if (data2)
		for (char i = 0; i < 41; ++i)
			data2[i] = 14 + i;
	
	if (data)
	{
		printf("\tnow printing data\n");
		for (size_t i = 0; i < 14; ++i)
			printf("data[%d] = %d\n", i, data[i]);
	}

	if (data2)
	{
		printf("\tnow printing data2\n");
		for (size_t i = 0; i < 41; ++i)
			printf("data2[%d] = %d\n", i, data2[i]);
	}
	
	if (data) MEMORY.free(data);
	if (data2) MEMORY.free(data2);

	engineState = ENGINE.shutDown();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to shutdown");

	return EXIT_SUCCESS;
}
