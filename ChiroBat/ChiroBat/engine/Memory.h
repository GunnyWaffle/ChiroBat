#ifndef CHIROBAT_MEMORY
#define CHIROBAT_MEMORY

#include "Types.h"
#include "Patterns.h"

#define MEMORY_POOL_SIZE (1024 * 1024)

namespace ChiroBat
{
	namespace Memory
	{
		struct Block
		{
			byte* root;
			uint32_t size;
		};

		class MemoryManager : public Patterns::Singleton<MemoryManager>
		{
		public:
			funcRet init();
			funcRet shutDown();
			
			template<typename T>
			void* alloc();
		private:
			byte* pool;
		};

		template<typename T>
		void* MemoryManager::alloc()
		{
			return nullptr;
		}
	}
}

#endif
