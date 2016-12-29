#ifndef CHIROBAT_ENGINE
#define CHIROBAT_ENGINE

#include "Types.h"
#include "Patterns.h"

#define ENGINE ChiroBat::Engine::Engine::instance()

namespace ChiroBat
{
	namespace Engine
	{
		class Engine : public Patterns::Singleton<Engine>
		{
		public:
			funcRet init();
			funcRet shutDown();
		};
	}
}

#endif
