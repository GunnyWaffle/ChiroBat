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
			funcRet init(size_t poolSize, bool expand);
			funcRet shutDown();

			template <typename T>
			int findMSB(T n);
			template <typename T>
			int findLSB(T n);

			void* malloc(size_t size);
			void* calloc(size_t size);
			void* alignMalloc(size_t size, size_t align);
			void* alignCalloc(size_t size, size_t align);
			funcRet free(void* pointer);
			
		private:
			struct Block;
			struct FreeList // packed linker pointers
			{
				Block* prev; // previous block in this bin
				Block* next; // next block in this bin
			};
			struct Block // a block of memory, be it free or used
			{
				Block* neighbor; // preceeding physical block
				size_t size; // block size with bitPacking(0x2 = "neighbor is free" 0x1 = "I am free")
				union
				{
					FreeList free; // doubly linked list of free blocks in a bin
					byte data; // user data, this is the pointer used by the end user
				} block; // block data, the memory block (in multiple states)
			};
			union Pool // double description of a pool
			{
				Block block; // handles shoving the pool into the free block manager
				Pool* prevPool; // allows cleanup of pools, a singley linked list ending in nullptr
			};
			struct MapIndex // container for navigating the free blocks map
			{
				short bin; // the bin index, fl * SLGranularity + sl
				byte fl; // the first layer index
				byte sl; // the second layer index
			};

			size_t maxRequestSize;
			size_t poolSize;

			byte SLgranularity;
			byte SLbitDepth;
			byte bitPack;
			byte packedFLI;
			size_t bitPackMask;
			byte minBlockSize;
			byte expand;

			Pool* pool;
			Block** freeBlocks;
			size_t flMask;
			size_t* slMasks;

			funcRet addPool();
			void addBlock(Block* block);
			void removeBlock(Block* block);
			funcRet splitBlock(Block* block, size_t size);
			funcRet getNextBlock(Block* block, Block** next);
			funcRet getIndex(size_t size, MapIndex* index);
			Block* getBlock(size_t size);
			size_t blockSize(Block* block);
		};

		template <typename T>
		int MemoryManager::findMSB(T n)
		{
			if (!n)
				return -1;

			constexpr int mid = (sizeof(T) << 2) - 1;
			constexpr int index = sizeof(T) - (sizeof(T) == 4) - ((sizeof(T) == 8) << 2);
			constexpr int steps[6] = { 1, 2, 4, 8, 16, 32 };
			int ret = mid;
			int i = index;
			T test;

			while ((test = n >> ret) - 1)
			{
				ret -= steps[i] - (steps[i + 1] & (!test - 1));
				i -= !!i;
			}

			return ret;
		}

		template <typename T>
		int MemoryManager::findLSB(T n)
		{
			return findMSB(n & (~n + 1));
		}
	}
}

#endif
