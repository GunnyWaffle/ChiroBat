#include <malloc.h>
#include <string.h>
#include "Memory.h"
#include "Debug.h"

namespace ChiroBat
{
	namespace Memory
	{
		funcRet MemoryManager::init(size_t poolSize)
		{
			RET_ON_ERR(pool, EXIT_FAILURE, "[Memory Manager] re-initialization of the manager was attempted");

			SLgranularity = sizeof(void*) * 8; // 32 | 64
			SLbitDepth = findMSB(SLgranularity); // 5 | 6
			bitPack = findMSB(sizeof(void*)); // 2 | 3
			packedFLI = SLbitDepth + bitPack - 1; // 6 | 8

			bitPackMask = ~(size_t)0 << bitPack;

			FLmax = findMSB(poolSize);

			maxRequestSize = (((size_t)1 << (FLmax + 1)) - 1) & bitPackMask;

			MapIndex index;
			getIndex(maxRequestSize, &index);
			FLmax = index.fl;

			this->poolSize = offsetof(Block, block.data) + maxRequestSize + sizeof(size_t);

			freeBlocks = (Block**)_mm_malloc((FLmax + 1) * SLgranularity * sizeof(Block*), alignof(Block*));
			slMasks = (size_t*)_mm_malloc((FLmax + 1) * sizeof(size_t), alignof(size_t));
			memset(freeBlocks, 0, (FLmax + 1) * SLgranularity * sizeof(Block*));
			memset(slMasks, 0, (FLmax + 1) * sizeof(size_t));
			flMask = 0;

			pool = nullptr;
			addPool();

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::shutDown()
		{
			RET_ON_ERR(!pool, EXIT_FAILURE, "[Memory Manager] shutdown of the non-initialized manager was attempted");

			Pool* temp;
			while (pool->prevPool)
			{
				temp = pool->prevPool;
				_mm_free(pool);
				pool = temp;
			}

			_mm_free(pool);
			_mm_free(freeBlocks);
			_mm_free(slMasks);

			pool = nullptr;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::findMSB(size_t n)
		{
			if (!n)
				return -1;

			funcRet ret = sizeof(size_t) * 4 - 1;
			int step = (ret + 1) >> 1;
			size_t seeker = (size_t)1 << ret;
			
			while (!(seeker <= n && (seeker << 1 > n || seeker << 1 == 0)))
			{
				if(step)
					ret += step * ((seeker <= n) * 2 - 1);
				else
					ret += ((seeker <= n) * 2 - 1);
				step >>= 1;
				seeker = (size_t)1 << ret;
			}

			return ret;
		}

		funcRet MemoryManager::findLSB(size_t n)
		{
			return findMSB(n & (~n + 1));
		}

		void* MemoryManager::malloc(size_t size)
		{
			if (size != (size & bitPackMask))
				size += (~size & ~bitPackMask) + 1;

			size = size < sizeof(Block*) * 3 ? sizeof(Block*) * 3 : size;

			RET_ON_ERR(size > maxRequestSize, nullptr, "[Memory Manager] malloc size request of %zu exceeded maximum request size of %zu", size, maxRequestSize);

			MapIndex index;
			getIndex(size, &index);

			Block* block = freeBlocks[index.bin];

			while (block && blockSize(block) < size)
				block = block->block.free.next;

			if (!block)
			{
				if (findNextUsedBin(&index))
				{
					RET_ON_ERR(addPool(), nullptr, "[Memory Manager] malloc failed to allocate block of size %zu", size);
					findNextUsedBin(&index);
				}
				block = freeBlocks[index.bin];
			}
			
			splitBlock(block, size);

			block->size &= ~(size_t)1;

			Block* next;
			if (!getNextBlock(block, &next))
				next->size &= ~(size_t)2;
			
			return &block->block.data;
		}

		void* MemoryManager::calloc(size_t size)
		{
			void* ret = malloc(size);

			if (ret)
				memset(ret, 0, size);

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
			RET_ON_ERR(!pointer, EXIT_FAILURE, "[Memory Manager] attempted to free a NULL pointer");

			Block* block = (Block*)((byte*)pointer - offsetof(Block, block.data));
			
			if (block->size & 2)
			{
				removeBlock(block->neighbor);
				block->neighbor->size += sizeof(block->size) + blockSize(block);
				block = block->neighbor;
			}

			Block* next;

			if (!getNextBlock(block, &next) && next->size & 1)
			{
				removeBlock(next);
				block->size += sizeof(next->size) + blockSize(next);
			}

			if (!getNextBlock(block, &next))
			{
				next->size |= 2;
				next->neighbor = block;
			}

			addBlock(block);

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::addPool()
		{
			RET_ON_ERR(!poolSize, EXIT_FAILURE, "[Memory Manager] attempted to expand the pool chain with no pool size");

			Pool* temp = (Pool*)_mm_malloc(poolSize, alignof(Pool*));
			RET_ON_ERR(!temp, EXIT_FAILURE, "[Memory Manager] failed to allocate new memory pool");
			temp->prevPool = pool;
			pool = temp;

			pool->block.size = maxRequestSize;
			addBlock(&pool->block);

			*(size_t*)((byte*)pool + poolSize - sizeof(size_t)) = 0; // fake block of size 0

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::addBlock(Block* block)
		{
			MapIndex index;
			getIndex(blockSize(block), &index);

			Block* head = freeBlocks[index.bin];

			if (!head)
			{
				freeBlocks[index.bin] = block;
				block->block.free.prev = nullptr;
				block->block.free.next = nullptr;
			}
			else
			{
				while (blockSize(head) < blockSize(block) && head->block.free.next)
					head = head->block.free.next;
				
				block->block.free.next = head->block.free.next;
				head->block.free.next = block;
				block->block.free.prev = head;

				if (block->block.free.next)
					block->block.free.next->block.free.prev = block;
			}

			block->size |= 1;

			flMask |= (size_t)1 << index.fl;
			slMasks[index.fl] |= (size_t)1 << index.sl;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::removeBlock(Block* block)
		{
			MapIndex index;
			getIndex(blockSize(block), &index);

			if (block->block.free.prev)
				block->block.free.prev->block.free.next = block->block.free.next;
			if (block->block.free.next)
				block->block.free.next->block.free.prev = block->block.free.prev;

			if (freeBlocks[index.bin] == block)
				freeBlocks[index.bin] = block->block.free.next;

			slMasks[index.fl] &= ~(size_t)(freeBlocks[index.bin] == nullptr) << index.sl;
			flMask &= ~(size_t)(slMasks[index.fl] == 0) << index.fl;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::splitBlock(Block* block, size_t size)
		{
			if (blockSize(block) - size < sizeof(size_t) * 2)
				return EXIT_FAILURE;
			
			size_t oldSize = blockSize(block);
			removeBlock(block);
			block->size = size;

			Block* newBlock;
			getNextBlock(block, &newBlock); // return ignored because garbage data
			
			newBlock->size = oldSize - size - sizeof(size_t);

			addBlock(newBlock);
			
			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::getNextBlock(Block* block, Block** next)
		{
			*next = (Block*)((byte*)block + blockSize(block) + offsetof(Block, size));

			if (!blockSize(*next))
				return EXIT_FAILURE;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::getIndex(size_t size, MapIndex* index)
		{
			index->fl = findMSB(size);

			if (index->fl <= packedFLI)
			{
				index->sl = (byte)(((size ^ ((size_t)1 << index->fl)) >> bitPack) + ((size_t)1 << (index->fl - bitPack)) - 1);
				index->fl = 0;
			}
			else
			{
				index->sl = (byte)(size >> (index->fl - SLbitDepth) ^ ((size_t)1 << SLbitDepth));
				index->fl -= packedFLI;
			}

			index->bin = index->fl * SLgranularity + index->sl;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::findNextUsedBin(MapIndex* index)
		{
			size_t newSLMask = slMasks[index->fl] & ~(((size_t)1 << (index->sl + 1)) - 1);
			
			if (!newSLMask)
			{
				size_t newFLMask = flMask & ~(((size_t)1 << (index->fl + 1)) - 1);

				RET_ON_ERR(!newFLMask, EXIT_FAILURE, "[Memory Manager] failed to find next used bin, a new pool needs to be added");

				index->fl = findLSB(newFLMask);
				newSLMask = slMasks[index->fl];
			}

			index->sl = findLSB(newSLMask);
			index->bin = index->fl * SLgranularity + index->sl;

			return EXIT_SUCCESS;
		}

		size_t MemoryManager::blockSize(Block* block)
		{
			return block->size & bitPackMask;
		}
	}
}