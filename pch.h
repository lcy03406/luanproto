#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#ifdef _MSC_VER
#include <io.h>
#define KJ_STD_COMPAT
#pragma warning(disable:4996)
#pragma warning(push)
#pragma warning(disable:4244) // conversion from 'unsigned __int64' to 'capnp::uint', possible loss of data
#pragma warning(disable:4267) // conversion from 'size_t' to 'capnp::uint', possible loss of data
#pragma warning(disable:4521) // multiple copy constructors specified
#pragma warning(disable:4800) // forcing value to bool 'true' or 'false' (performance warning)
#endif

#include <kj/debug.h>
#include <kj/io.h>
#include <kj/one-of.h>

#define CAPNP_PRIVATE
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/pointer-helpers.h>
#include <capnp/dynamic.h>
#include <capnp/layout.h>
#include <capnp/arena.h>
#include <capnp/schema-parser.h>
#include <capnp/pretty-print.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <lua/lua.hpp>
#if LUA_VERSION_NUM<502
#define lua_rawlen lua_objlen
#endif

#ifdef _WIN32 // TODO add options to static linkage
#define LIBLUACAPNP_API __declspec(dllexport)
#else
#define LIBLUACAPNP_API 
#endif

#define LUA_INT_IS_TOO_SMALL sizeof(lua_Integer)<sizeof(int64_t)
#ifndef LUA_MAXINTEGER
#define LUA_MAXINTEGER INT_MAX
#define LUA_MININTEGER INT_MIN
#endif

#define LuaTryCatch LuaCapnpTryCatch

template<typename Func>
inline int LuaTryCatch(lua_State *L, Func&& func)
{
	int ret = 0;
	bool jump = false;
	lua_gc(L, LUA_GCSTOP, 0);
	//make sure cb is destructed before longjmp
	//TODO for now, func() must not raise any lua error
	{
		KJ_IF_MAYBE(e, kj::runCatchingExceptions([&]()
		{
			ret = func();
		}))
		{
			auto desc = e->getDescription();
			lua_pushlstring(L, desc.cStr(), desc.size());
			jump = true;
		}
	}
	lua_gc(L, LUA_GCRESTART, 0);
	if (jump)
	{
		return lua_error(L);
	}
	return ret;
}

