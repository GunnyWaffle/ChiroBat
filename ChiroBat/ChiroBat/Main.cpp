#include "engine\Engine.h"
#include "engine\Debug.h"
#include "engine\Memory.h"

using namespace ChiroBat;

#define MAKE_DATA(type, count) (type*)MEMORY.malloc(sizeof(type) * count)
#define MAKE_DATA_ZERO(type, count) (type*)MEMORY.calloc(sizeof(type) * count)

#define SET_DATA(data, count, offset) \
do \
{ \
if (data) \
for (size_t i = 0; i < count; ++i) \
	data[i] = offset + i; \
} while(0)

#define PRINT_DATA(data, count) \
do \
{ \
if (data) \
{ \
	printf("\tnow printing %s\n", #data); \
	for (size_t i = 0; i < count; ++i) \
		printf("%s[%d] = %d\n", #data, i, data[i]); \
} \
} while(0)

#define FREE_DATA(data) if(data) MEMORY.free(data)

int main()
{
	funcRet engineState;

	engineState = ENGINE.init();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to initialize");

	int* data = MAKE_DATA(int, 14);
	char* data2 = MAKE_DATA_ZERO(char, 41);
	
	SET_DATA(data, 14, 0);
	SET_DATA(data2, 41, 14);
	
	PRINT_DATA(data, 14);
	PRINT_DATA(data2, 41);
	
	FREE_DATA(data);
	FREE_DATA(data2);

	engineState = ENGINE.shutDown();
	RET_ON_ERR(engineState, EXIT_FAILURE, "[Main] Engine failed to shutdown");

	return EXIT_SUCCESS;
}
