#include "PCH.h"
#include "Lib.h"
#include "battle/Battle.h"
#include "battle/BehaviorMeta.h"
#include "physics/PhysicsTester.h"

#ifdef _WINDOWS
#include <Windows.h>
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

namespace BlackWood
{

	extern "C"
	{
		LIBBATTLE_API int LibInit(const char* respath, PrintLogFunc printlog)
		{
			Logger::Init(printlog);
			int ret = 0;
			ret = AllTemplate::Load(respath);
			if (ret != 0)
				return ret;
			BehaviorMeta::Init();
			return 0;
		}

		void SetLogLevel(const char* module, int level)
		{
			Logger::SetLevel(module, (LogLevel)level);
		}

		LIBBATTLE_API Battle* BattleNew(BattleOutputFunc o)
		{
			auto battle = new Battle(o);
			return battle;
		}

		LIBBATTLE_API int BattleInput(Battle* battle, const uint8_t* indata, int insize)
		{
			return (int)battle->Input((const ::capnp::word*)indata, insize/sizeof(::capnp::word));
		}

		LIBBATTLE_API void BattleDelete(Battle* battle)
		{
			delete battle;
		}

		LIBBATTLE_API BlackWood::Physics::PhysicsTester* test_start()
		{
			BlackWood::Physics::PhysicsTester* tester = new BlackWood::Physics::PhysicsTester();
			return tester;
		}
		LIBBATTLE_API void test_update(BlackWood::Physics::PhysicsTester* tester, BlackWood::Physics::FixedPhysicsWorld* wrold)
		{
			tester->Update(wrold);
		}
	}
}