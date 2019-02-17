#pragma once
#include "schema.h"

namespace luacapnp
{
	class Decode {
	public:
		static bool decode(lua_State *L, const schema::Struct& s, capnp::_::PointerReader& pointer);
	private:
		static void decodeStruct(lua_State *L, const schema::Struct& s, capnp::_::StructReader& p);
		static bool decodeField(lua_State *L, const schema::Struct& s, capnp::_::StructReader& p, const char* fname);
		static bool decodeSlot(lua_State *L, const schema::SlotField& ss, capnp::_::StructReader& p);
		static bool decodeValue(lua_State *L, const schema::SlotField& ss, capnp::_::StructReader& p);
		static void decodeDepth(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr, int depth);
		static void decodeList(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr);
		static void decodeSet(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr);
		static void decodeMap(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr);
	};
}
