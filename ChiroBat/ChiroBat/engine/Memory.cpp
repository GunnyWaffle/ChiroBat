#include <malloc.h>
#include <string.h>
#include "Memory.h"
#include "Debug.h"

namespace ChiroBat
{
	namespace Memory
	{
		funcRet MemoryManager::init(size_t poolSize, bool expand)
		{
			// prevent over-initialization
			RET_ON_ERR(pool, EXIT_FAILURE, "[Memory Manager] re-initialization of the manager was attempted");

			// compute the system's needs, 32 | 64
			SLgranularity = sizeof(void*) * 8; // 32 | 64
			SLbitDepth = findMSB(SLgranularity); // 5 | 6
			bitPack = findMSB(sizeof(void*)); // 2 | 3
			packedFLI = SLbitDepth + bitPack - 1; // 6 | 8
			bitPackMask = ~(size_t)0 << bitPack;

			// clamp the pool to allow the block to fit
			poolSize = poolSize < sizeof(Pool) ? sizeof(Pool) : poolSize;

			// determine the maximum first layer size (un-adjusted)
			byte FLmax = findMSB(poolSize);

			// find the largest block allowed in the free blocks array
			maxRequestSize = (((size_t)1 << (FLmax + 1)) - 1) & bitPackMask;
			// and minimum
			minBlockSize = sizeof(Block*) * 3;

			// calculate the adjusted layer sizes
			MapIndex index;
			getIndex(maxRequestSize, &index);
			FLmax = index.fl;

			// how big a pool needs to be to accomodate the block data, rewuested size, and fake block at the end
			this->poolSize = offsetof(Block, block.data) + maxRequestSize + sizeof(size_t);

			// allocate the structure onto the system HEAP
			// TODO move this into the front of the pool
			freeBlocks = (Block**)_mm_malloc((FLmax + 1) * SLgranularity * sizeof(Block*), alignof(Block*));
			slMasks = (size_t*)_mm_malloc((FLmax + 1) * sizeof(size_t), alignof(size_t));
			memset(freeBlocks, 0, (FLmax + 1) * SLgranularity * sizeof(Block*));
			memset(slMasks, 0, (FLmax + 1) * sizeof(size_t));
			flMask = 0;

			// allocate a pool on the system HEAP
			this->expand = expand;
			pool = nullptr;
			addPool();

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::shutDown()
		{
			// there is nothing to shut down if this is true
			RET_ON_ERR(!pool, EXIT_FAILURE, "[Memory Manager] shutdown of the non-initialized manager was attempted");

			// run through the pool list
			Pool* temp;
			while (pool->prevPool) // while there is a previous pool
			{
				temp = pool->prevPool; // track where you are
				_mm_free(pool); // free this pool
				pool = temp; // move to the tracker
			}

			// free the structure
			_mm_free(pool);
			_mm_free(freeBlocks);
			_mm_free(slMasks);

			// the pool is now officially gone
			pool = nullptr;

			return EXIT_SUCCESS;
		}

		void* MemoryManager::malloc(size_t size)
		{
			if (size != (size & bitPackMask)) // if the size is not aligned to the mask
				size += (~size & ~bitPackMask) + 1; // align it to the mask

			// the size exceeds the tolerated range
			RET_ON_ERR(size > maxRequestSize, nullptr, "[Memory Manager] malloc size request of %zu exceeds maximum request size of %zu", size, maxRequestSize);

			// ensure the size is at least minimum size
			size = size < minBlockSize ? minBlockSize : size;

			Block* block = getBlock(size); // attempt to get a block
			if (!block) // no block found
			{
				// add a pool
				RET_ON_ERR(addPool(), nullptr, "[Memory Manager] malloc failed to allocate block of size %zu", size);
				block = getBlock(size); // get the new block
			}
			
			splitBlock(block, size); // split if possible, to maximise memory usage
			removeBlock(block); // remove this block from the free blocks array
			
			return &block->block.data; // give the user the pointer to the data portion of the block
		}

		void* MemoryManager::calloc(size_t size)
		{
			void* ret = malloc(size); // malloc the request

			if (ret) // if it succeeded
				memset(ret, 0, size); // memset to 0

			return ret;
		}

		void* MemoryManager::alignMalloc(size_t size, size_t align)
		{
			RET_ON_ERR(1, nullptr, "[Memory Manager] aligned malloc is not yet implemented, no address will be given out");

			return nullptr;
		}

		void* MemoryManager::alignCalloc(size_t size, size_t align)
		{
			void* ret = alignMalloc(size, align);

			if (ret)
				memset(ret, 0, size);

			return ret;
		}

		funcRet MemoryManager::free(void* pointer)
		{
			// avoid null pointers
			RET_ON_ERR(!pointer, EXIT_FAILURE, "[Memory Manager] attempted to free a NULL pointer");

			// extract the block from the pointer
			Block* block = (Block*)((byte*)pointer - offsetof(Block, block.data));
			
			if (block->size & 2) // is the previous neighbor free?
			{
				removeBlock(block->neighbor); // remove it from the free blocks array
				block->neighbor->size += sizeof(block->size) + blockSize(block); // expand it to encapsulate this block
				block = block->neighbor; // move to it
			}

			Block* next;

			// get the next block, and check if it is free
			if (!getNextBlock(block, &next) && next->size & 1)
			{
				removeBlock(next); // remove it
				block->size += sizeof(next->size) + blockSize(next); // encapsulate it
			}

			addBlock(block); // add this block to the free blocks array

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::addPool()
		{
			// check if expansion is valid
			RET_ON_ERR(!expand && pool, EXIT_FAILURE, "[Memory Manager] the allocator is set to not expand, no more memory can be added");
			RET_ON_ERR(!poolSize, EXIT_FAILURE, "[Memory Manager] attempted to expand the pool chain with no pool size");

			// allocate a new pool on the system HEAP
			Pool* temp = (Pool*)_mm_malloc(poolSize, alignof(Pool*));
			RET_ON_ERR(!temp, EXIT_FAILURE, "[Memory Manager] failed to allocate new memory pool");
			temp->prevPool = pool; // link it to the current pool
			pool = temp; // make it the current pool

			// add the pool to the free blocks array
			pool->block.size = maxRequestSize;
			addBlock(&pool->block);

			// cap off the pool to prevent the "next block" code from overstepping the pool
			*(size_t*)((byte*)pool + poolSize - sizeof(size_t)) = 0; // fake block of size 0

			return EXIT_SUCCESS;
		}

		void MemoryManager::addBlock(Block* block)
		{
			MapIndex index; // get the bin for the block
			getIndex(blockSize(block), &index);

			Block* head = freeBlocks[index.bin]; // get the bin's head

			if (!head) // if there is no head
			{
				freeBlocks[index.bin] = block; // this block is the head
				block->block.free.prev = nullptr;
				block->block.free.next = nullptr;
			}
			else // there are blocks in this bin
			{
				// search for where this block belongs (best-fit order)
				while (blockSize(head) < blockSize(block) && head->block.free.next)
					head = head->block.free.next;
				
				// merge it into the doubley linked list
				block->block.free.next = head->block.free.next;
				head->block.free.next = block;
				block->block.free.prev = head;

				// if there is a next block, link it
				if (block->block.free.next)
					block->block.free.next->block.free.prev = block;
			}

			// this block is now flagged as free
			block->size |= 1;

			Block* next; // get the next block
			if (!getNextBlock(block, &next))
			{
				next->size |= 2; // tell it that this block is free
				next->neighbor = block; // and where it is
			}

			flMask |= (size_t)1 << index.fl; // flag this layer as being occupied
			slMasks[index.fl] |= (size_t)1 << index.sl; // flag this bin as being occupied
		}

		void MemoryManager::removeBlock(Block* block)
		{
			MapIndex index; // get the bin for the block
			getIndex(blockSize(block), &index);

			// unlink the previous block
			if (block->block.free.prev)
				block->block.free.prev->block.free.next = block->block.free.next;
			// unlink the next block
			if (block->block.free.next)
				block->block.free.next->block.free.prev = block->block.free.prev;

			// update the bin head if this block is the head
			if (freeBlocks[index.bin] == block)
				freeBlocks[index.bin] = block->block.free.next;

			// flag this layer as being empty if need be
			slMasks[index.fl] &= ~(size_t)(freeBlocks[index.bin] == nullptr) << index.sl;
			// flag this bin as being empty if need be
			flMask &= ~(size_t)(slMasks[index.fl] == 0) << index.fl;

			// flag this block as being used
			block->size &= ~(size_t)1;

			Block* next; // get the next block
			if (!getNextBlock(block, &next))
				next->size &= ~(size_t)2; // tell it that this block is being used
		}

		funcRet MemoryManager::splitBlock(Block* block, size_t size)
		{
			// only split if there is room to split
			if (blockSize(block) - size < sizeof(Block) - sizeof(Block*))
				return EXIT_FAILURE;
			
			size_t oldSize = blockSize(block); // the current memory to split
			removeBlock(block); // remove this block from the free blocks array
			block->size = size; // give it the new size

			Block* newBlock; // find the splitting block

			// return ignored because garbage data
			// TODO this is a logical leak, in theory the cap can be found this way
			// TODO can't rely on the size 0, because there could be 0s in the block
			// oh boy...
			getNextBlock(block, &newBlock);
			
			// inform it of its new size
			// TODO if this is the cap, this just corrupted the pool
			newBlock->size = oldSize - size - sizeof(size_t);

			// re-introduce the blocks to the free blocks array
			addBlock(block);
			addBlock(newBlock);
			
			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::getNextBlock(Block* block, Block** next)
		{
			// grab the next block, given size of the current block
			*next = (Block*)((byte*)block + blockSize(block) + offsetof(Block, size));

			// return failure if the next block is the fake cap block
			if (!blockSize(*next))
				return EXIT_FAILURE;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::getIndex(size_t size, MapIndex* index)
		{
			index->fl = findMSB(size); // get the first layer index from the size

			if (index->fl <= packedFLI) // adjustment is needed, packed second layers
			{
				// get the second layer index for packed layering
				index->sl = (byte)(((size ^ ((size_t)1 << index->fl)) >> bitPack) + ((size_t)1 << (index->fl - bitPack)) - 1);
				index->fl = 0; // all packed layers are in first layer 0
			}
			else // standard protocol
			{
				// the second layer is the SLbitDepth bits before the MSB
				index->sl = (byte)(size >> (index->fl - SLbitDepth) ^ ((size_t)1 << SLbitDepth));
				index->fl -= packedFLI; // offset for the packed 0 layer
			}

			// pre-computed bin index for simplicity
			index->bin = index->fl * SLgranularity + index->sl;

			return EXIT_SUCCESS;
		}

		MemoryManager::Block* MemoryManager::getBlock(size_t size)
		{
			MapIndex index; // get the bin for the block
			getIndex(size, &index);

			Block* block = freeBlocks[index.bin]; // get the block

			// look for the best-fit block
			while (block && blockSize(block) < size)
				block = block->block.free.next;

			if (!block) // if no block is found
			{
				// move up in the second layer
				size_t newSLMask = slMasks[index.fl] & ~(((size_t)1 << (index.sl + 1)) - 1);

				if (!newSLMask) // if there is nothing left in this layer
				{
					// move up in the first layer
					size_t newFLMask = flMask & ~(((size_t)1 << (index.fl + 1)) - 1);

					// if there is nothing left at all
					RET_ON_ERR(!newFLMask, nullptr, "[Memory Manager] failed to find a free block, a new pool will be added");

					// use this new first layer and get its second layer mask
					index.fl = findLSB(newFLMask);
					newSLMask = slMasks[index.fl];
				}

				// the block can be found now
				block = freeBlocks[index.fl * SLgranularity + findLSB(newSLMask)];
			}

			return block;
		}

		size_t MemoryManager::blockSize(Block* block)
		{
			// mask away the bit logic to get the actual size
			return block->size & bitPackMask;
		}
	}
}