#pragma once
#include "Dll.h"

namespace BlackWood
{
	class Battle;
	namespace Physics
	{
		class PhysicsTester;
		class FixedPhysicsWorld;
	}

	extern "C"
	{
		typedef void(__stdcall *BattleOutputFunc)(const uint8_t* data, int size);
		typedef void(__stdcall *PrintLogFunc)(const char *file, int line, const char* module, int level, const char* text);
		LIBBATTLE_API int LibInit(const char* respath, PrintLogFunc printlog);
		LIBBATTLE_API void SetLogLevel(const char* module, int level);
		LIBBATTLE_API Battle* BattleNew(BattleOutputFunc);
		LIBBATTLE_API int BattleInput(Battle* battle_, const uint8_t* indata, int insize);
		LIBBATTLE_API void BattleDelete(Battle* battle);

		LIBBATTLE_API BlackWood::Physics::PhysicsTester* test_start();
		LIBBATTLE_API void test_update(BlackWood::Physics::PhysicsTester* tester, BlackWood::Physics::FixedPhysicsWorld* wrold);
	}
}