#ifndef CHIROBAT_PATTERNS
#define CHIROBAT_PATTERNS

namespace ChiroBat
{
	namespace Patterns
	{
		template <class T>
		class Singleton
		{
		public:
			static T& instance()
			{
				static Instance instance_object;
				return instance_object;
			}

		protected:
			Singleton& operator=(const Singleton&) = delete;

		private:
			virtual void SingletonHook() = 0;
			class Instance : public T
			{
			private:
				void SingletonHook() {}
				Instance& operator=(const Instance&) = delete;
			};
		};
	}
}

#endif
