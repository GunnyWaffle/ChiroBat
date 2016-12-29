#ifndef CHIROBAT_MEMORY
#define CHIROBAT_MEMORY

#include "Types.h"
#include "Patterns.h"

#define MEMORY ChiroBat::Memory::MemoryManager::instance()

namespace ChiroBat
{
	namespace Memory
	{
		class MemoryManager : public Patterns::Singleton<MemoryManager>
		{
		public:
			funcRet init(size_t poolSize);
			funcRet shutDown();

			funcRet findMSB(size_t n);
			funcRet findLSB(size_t n);

			funcRet malloc(size_t size, void** pointer);
			funcRet calloc(size_t size, void** pointer);
			funcRet alignMalloc(size_t size, size_t align, void** pointer);
			funcRet alignCalloc(size_t size, size_t align, void** pointer);
			funcRet free(void* pointer);
			
		private:
			struct Block;
			struct FreeList
			{
				Block* prev;
				Block* next;
			};
			struct Block
			{
				Block* neighbor; // preceeding physical block
				size_t size; // block size
				union
				{
					FreeList free; // doubly linked list of free blocks
					byte data; // user data
				} block; // block data
			};
			union Pool
			{
				Block block;
				Pool* prevPool;
			};
			struct MapIndex
			{
				short bin;
				byte fl;
				byte sl;
			};

			size_t maxRequestSize;
			size_t poolSize;

			byte SLgranularity;
			byte SLbitDepth;
			byte bitPack;
			byte packedFLI;
			size_t bitPackMask;
			byte FLmax;

			Pool* pool;
			Block** freeBlocks;
			size_t flMask;
			size_t* slMasks;

			funcRet addPool();
			funcRet addBlock(Block* block);
			funcRet removeBlock(Block* block);
			funcRet getIndex(size_t size, MapIndex* index);
		};
	}
}

#endif
