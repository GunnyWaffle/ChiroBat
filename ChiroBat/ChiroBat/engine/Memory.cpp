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
			int step = ret + 1;
			size_t seeker = (size_t)1 << ret;
			
			while (!(seeker <= n && seeker << 1 > n))
				seeker = (size_t)1 << (ret += (step >>= 1) * ((seeker <= n) * 2 - 1));

			return ret;
		}

		funcRet MemoryManager::findLSB(size_t n)
		{
			return findMSB(n & (~n + 1));
		}

		funcRet MemoryManager::malloc(size_t size, void** pointer)
		{
			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::calloc(size_t size, void** pointer)
		{
			funcRet ret = malloc(size, pointer);

			if (!ret)
				memset(*pointer, 0, size);

			return ret;
		}

		funcRet MemoryManager::alignMalloc(size_t size, size_t align, void** pointer)
		{
			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::alignCalloc(size_t size, size_t align, void** pointer)
		{
			funcRet ret = alignMalloc(size, align, pointer);

			if (!ret)
				memset(*pointer, 0, size);

			return ret;
		}

		funcRet MemoryManager::free(void* pointer)
		{
			RET_ON_ERR(!pointer, EXIT_FAILURE, "[Memory Manager] attempted to free a NULL pointer");

			Block* block = (Block*)((byte*)pointer - offsetof(Block, block.data));

			if (block->size & 2)
			{
				removeBlock(block->neighbor);
				block->neighbor->size += sizeof(block->size) + block->size;
				block = block->neighbor;
			}

			Block* next = (Block*)((byte*)block + block->size - offsetof(Block, size));

			if (next->size && next->size & 1)
			{
				removeBlock(next);
				block->size += sizeof(next->size) + next->size;
			}

			addBlock(block);

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::addPool()
		{
			RET_ON_ERR(!poolSize, EXIT_FAILURE, "[Memory Manager] attempted to expand the pool chain with no pool size");

			Pool* temp = (Pool*)_mm_malloc(poolSize, alignof(Pool*));
			RET_ON_ERR(!temp, EXIT_FAILURE, "[Memory Manager] failed to allocate memory pool");
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
			getIndex(block->size, &index);

			Block* head = freeBlocks[index.bin];

			if (!head)
			{
				freeBlocks[index.bin] = block;
				block->block.free.prev = nullptr;
				block->block.free.next = nullptr;
			}
			else
			{
				while (head->size < block->size && head->block.free.next)
					head = head->block.free.next;
				
				block->block.free.next = head->block.free.next;
				head->block.free.next = block;
				block->block.free.prev = head;

				if (block->block.free.next)
					block->block.free.next->block.free.prev = block;
			}

			flMask |= (size_t)1 << index.fl;
			slMasks[index.fl] |= (size_t)1 << index.sl;

			return EXIT_SUCCESS;
		}

		funcRet MemoryManager::removeBlock(Block* block)
		{
			MapIndex index;
			getIndex(block->size, &index);

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
	}
}