#pragma once
#include "schema.h"

namespace luacapnp
{
	class Dynamic {
	public:
		explicit Dynamic(schema::Registry& registry);
		int interface(lua_State *L);
		int encode(lua_State *L);
		int decode(lua_State *L);
		int pretty(lua_State *L);
		int serialize(lua_State *L);
		int deserialize(lua_State *L);
		int text(lua_State *L);
		int parse(lua_State *L);
		
	private:
		const schema::Struct* findSchema(lua_State *L, int index, int *ordinal = nullptr, kj::StringPtr* name = nullptr);
		const schema::Struct* findStructSchema(lua_State *L, int index);

		schema::Registry& registry;
	};
}
