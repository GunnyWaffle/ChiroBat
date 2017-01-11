#ifndef CHIROBAT_MEMORY
#define CHIROBAT_MEMORY

#include "Types.h"
#include "Patterns.h"

#define MEMORY ChiroBat::Memory::MemoryManager::instance()

// This allocator is heavily based upon the TLSF memory allocation approach
// implemented from reading the white papers

namespace ChiroBat
{
	namespace Memory
	{
		class MemoryManager : public Patterns::Singleton<MemoryManager>
		{
		public:
			// initialize the memory manager
			// poolsize - the poolsize to use, will be rounded up to the max supported from this size
			// expand - if true, new pools will be allocated as needed
			funcRet init(size_t poolSize, bool expand);
			funcRet shutDown(); // shutdown the memory manager

			template <typename T>
			int findMSB(T n); // find the index of the most significant bit
			template <typename T>
			int findLSB(T n); // find the index of the least significant bit

			void* malloc(size_t size); // allocate memory from the pool
			void* calloc(size_t size); // allocate memory from the pool, and init to 0s
			void* alignMalloc(size_t size, size_t align); // allocate aligned memory from the pool
			void* alignCalloc(size_t size, size_t align); // allocate aligned memory from the pool, and init to 0s
			funcRet free(void* pointer); // free memory allocated from the pool
			
		private:
			struct Block;
			struct FreeList // packed linked list pointers
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

			size_t maxRequestSize; // maximum memory request size for allocation
			size_t poolSize; // the size of each memory pool

			byte SLgranularity; // the bit size of the machine, used for the second layer granularity
			byte SLbitDepth; // the power of 2 needed for the bit size
			byte bitPack; // the number of bits in size lost to alignment, used for metadata
			byte packedFLI; // the minimum MSB value needed to exceed a first layer index of 0
			size_t bitPackMask; // helper value for masking out the unused bits in size
			byte minBlockSize; // minimum memory request size for allocation
			byte expand; // 1 - expand, 0 - do not expand

			Pool* pool; // the tail of the pool linked list
			Block** freeBlocks; // the free blocks array
			size_t flMask; // the first layer mask
			size_t* slMasks; // the second layer masks

			funcRet addPool(); // adds a new pool to the allocator
							   
			// adds a freed block to the free blocks array
			// block - the block to add
			void addBlock(Block* block);
			
			// removes a block from the free blocks array
			// block - the block to remove
			void removeBlock(Block* block);
			
			// split a block, if possible, adding the new block into the free blocks array
			// block - the block to split
			// size - the new size of the block
			funcRet splitBlock(Block* block, size_t size);

			// get the next neihgboring block in memory
			// block - the block to seek from
			// next - the block to assign to
			funcRet getNextBlock(Block* block, Block** next);

			// get the free blocks index for a size request
			// size - the block size needed
			// index - the index info to write to
			funcRet getIndex(size_t size, MapIndex* index);

			// get a block from the free blocks array
			// size - minimum size to search for
			Block* getBlock(size_t size);

			// get the size of a block
			// block - the block in question
			size_t blockSize(Block* block);
		};

		template <typename T>
		int MemoryManager::findMSB(T n)
		{
			if (!n) // 0 check case
				return -1;

			constexpr int mid = (sizeof(T) << 2) - 1; // middle of the binary tree
			constexpr int index = sizeof(T) - (sizeof(T) == 4) - ((sizeof(T) == 8) << 2); // stepping index
			constexpr int steps[6] = { 1, 2, 4, 8, 16, 32 }; // steps for the tree search
			int ret = mid; // start in the middle
			int i = index; // step from the index
			T test;

			while ((test = n >> ret) - 1) // while the MSB has not been found
			{
				ret -= steps[i] - (steps[i + 1] & (!test - 1)); // step
				i -= !!i; // decrement the step
			}

			return ret;
		}

		template <typename T>
		int MemoryManager::findLSB(T n)
		{
			return findMSB(n & (~n + 1)); // mask out the LSB and use MSB to find it
		}
	}
}

#endif
