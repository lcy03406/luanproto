#include "pch.h"
#include "decode.h"
#include "schema.h"
#include "helper.h"
#include "decode_helper.h"

namespace luacapnp
{
	bool Decode::decode(lua_State *L, const schema::Struct& s, capnp::_::PointerReader& pointer ) {
		if (pointer.isNull()) {
			return false;
		}
		auto p = pointer.getStruct(nullptr);
		decodeStruct(L, s, p);
		return true;
	}
	
	void Decode::decodeStruct(lua_State *L, const schema::Struct& s, capnp::_::StructReader& p) {
		uint16_t discriminant = 0;
		if (s.hasUnion) {
			discriminant = p.getDataField<uint16_t>(s.discriminantOffset * capnp::ELEMENTS);
		}
		lua_newtable(L);
		for (auto& iss : s.slots) {
			auto& name = iss.first;
			auto& ss = iss.second;
			if (ss.inUnion && ss.discriminantValue != discriminant) {
				continue;
			}
			lua_pushlstring(L, name.c_str(), name.size());
			if (decodeSlot(L, ss, p)) {
				lua_rawset(L, -3);
			} else {
				lua_pop(L, 1);
			}
		}
		for (auto& isg : s.groups) {
			auto& name = isg.first;
			auto& sg = isg.second;
			if (sg.inUnion && sg.discriminantValue != discriminant) {
				continue;
			}
			lua_pushlstring(L, name.c_str(), name.size());
			decodeStruct(L, sg.type, p);
			lua_rawset(L, -3);
		}
	}
	
	bool Decode::decodeField(lua_State *L, const schema::Struct& s, capnp::_::StructReader& p, const char* fname) {
		//needed by decodeMap, not decodeStruct
		auto iss = s.slots.find(fname);
		if (iss != s.slots.end()) {
			auto& ss = iss->second;
			return decodeSlot(L, ss, p);
		} else {
			auto isg = s.groups.find(fname);
			if (isg != s.groups.end()) {
				auto& sg = isg->second;
				decodeStruct(L, sg.type, p);
				return true;
			} else {
				//field not found
				return false;
			}
		}
	}

	
	bool Decode::decodeSlot(lua_State *L, const schema::SlotField& ss, capnp::_::StructReader& p) {
		const schema::Type& st = ss.type;
		if (st.depth == 0) {
			return decodeValue(L, ss, p);
		} else {
			capnp::_::PointerReader pr = p.getPointerField(ss.offset);
			if (pr.isNull()) {
				return false;
			}
			decodeDepth(L, ss, pr, 1);
			return true;
		}
	}
	
	bool Decode::decodeValue(lua_State *L, const schema::SlotField& ss, capnp::_::StructReader& p) {
		const schema::Type& st = ss.type;
		int offset = ss.offset;
		#define DECODE_VALUE(T) \
			case T: { \
				auto value = loadField<T>(p, offset); \
				pushValue<T>(L, value, st.enumType); \
				return true; \
			} break;
		
		switch (st.type) {
			FOR_ALL_TYPES(DECODE_VALUE)
			case capnp::schema::Type::LIST: {
				//imposible
				return false;
			} break;
			case capnp::schema::Type::STRUCT: {
				capnp::_::PointerReader pr = p.getPointerField(offset);
				return decode(L, *st.structType, pr);
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
				return false;
			} break;
		}
		#undef DECODE_VALUE
	}
	
	
	void Decode::decodeDepth(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr, int depth) {
		const schema::Type& st = ss.type;
		if (depth < st.depth) {
			capnp::_::ListReader lr = pr.getList(capnp::ElementSize::POINTER, nullptr);
			int len = lr.size();
			lua_createtable(L, len, 0);
			for (int i = 0; i < len; ++i) {
				capnp::_::PointerReader lpr = lr.getPointerElement(i);
				if (!lpr.isNull()) {
					decodeDepth(L, ss, lpr, depth+1);
					lua_rawseti(L, -2, i+1);
				}
			}
		} else {
			if (ss.ismap) {
				if (ss.type.type == capnp::schema::Type::STRUCT) {
					decodeMap(L, ss, pr);
				} else {
					decodeSet(L, ss, pr);
				}
			} else {
				decodeList(L, ss, pr);
			}
		}
	}

	void Decode::decodeList(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr) {
		const schema::Type& st = ss.type;

		#define DECODE_LIST(T) \
			case T: { \
				capnp::_::ListReader lr = pr.getList(CType<T>::Size, nullptr); \
				int len = lr.size(); \
				lua_createtable(L, len, 0); \
				for (int i = 0; i < len; ++i) { \
					auto value = loadListElement<T>(lr, i); \
					pushValue<T>(L, value, st.enumType); \
					lua_rawseti(L, -2, i+1); \
				} \
			} break;
		
		switch (st.type) {
			FOR_ALL_TYPES(DECODE_LIST)
			case capnp::schema::Type::LIST: {
				//imposible
				lua_pushnil(L);
			} break;
			case capnp::schema::Type::STRUCT: {
				capnp::_::ListReader lr = pr.getList(capnp::ElementSize::INLINE_COMPOSITE, nullptr);
				int len = lr.size();
				lua_createtable(L, len, 0);
				for (int i = 0; i < len; ++i) {
					auto sr = lr.getStructElement(i);
					decodeStruct(L, *st.structType, sr);
					lua_rawseti(L, -2, i+1);
				}
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
				lua_pushnil(L);
			} break;
		}
		#undef DECODE_LIST
	}

	void Decode::decodeSet(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr) {
		const schema::Type& st = ss.type;

		#define DECODE_SET(T) \
			case T: { \
				capnp::_::ListReader lr = pr.getList(CType<T>::Size, nullptr); \
				int len = lr.size(); \
				lua_createtable(L, 0, len); \
				for (int i = 0; i < len; ++i) { \
					auto value = loadListElement<T>(lr, i); \
					pushValue<T>(L, value, st.enumType); \
					lua_pushboolean(L, true); \
					lua_settable(L, -3); \
				} \
			} break;
		
		switch (st.type) {
			FOR_ALL_TYPES(DECODE_SET)
			case capnp::schema::Type::LIST: {
				//imposible
				lua_pushnil(L);
			} break;
			case capnp::schema::Type::STRUCT: {
				//imposible
				lua_pushnil(L);
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
				lua_pushnil(L);
			} break;
		}
		#undef DECODE_SET
	}

	void Decode::decodeMap(lua_State *L, const schema::SlotField& ss, capnp::_::PointerReader& pr) {
		const schema::Type& st = ss.type;
		capnp::_::ListReader lr = pr.getList(capnp::ElementSize::INLINE_COMPOSITE, nullptr);
		int len = lr.size();
		lua_createtable(L, 0, len);
		if (ss.mapvalue.empty()) {
			for (int i = 0; i < len; ++i) {
				auto sr = lr.getStructElement(i);
				decodeStruct(L, *st.structType, sr);
				lua_getfield(L, -1, ss.mapkey.c_str());
				lua_pushvalue(L, -2);
				lua_settable(L, -4);
				lua_pop(L, 1);
			}
		} else {
			for (int i = 0; i < len; ++i) {
				auto sr = lr.getStructElement(i);
				decodeField(L, *st.structType, sr, ss.mapkey.c_str());
				decodeField(L, *st.structType, sr, ss.mapvalue.c_str());
				lua_settable(L, -3);
			}
		}
	}
}

