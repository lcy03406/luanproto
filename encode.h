#pragma once
#include "schema.h"

namespace luacapnp
{
	class Encode {
	public:
		static void encode(lua_State *L, int index, const schema::Struct& s, capnp::_::PointerBuilder& pointer);
	private:
		static void encodeStruct(lua_State *L, int index, const schema::Struct& s, capnp::_::StructBuilder& p);
		static void encodeField(lua_State *L, int index, const schema::Struct& s, capnp::_::StructBuilder& p, const char* fname);
		static void encodeSlot(lua_State *L, int index, const schema::SlotField& ss, capnp::_::StructBuilder& p);
		static void encodeValue(lua_State *L, int index, const schema::SlotField& ss, capnp::_::StructBuilder& p);
		static void encodeDepth(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb, int depth);
		static void encodeList(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb);
		static void encodeSet(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb);
		static void encodeMap(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb);
	};
}
