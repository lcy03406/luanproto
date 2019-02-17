#include "pch.h"
#include "encode.h"
#include "schema.h"
#include "helper.h"
#include "encode_helper.h"

namespace luacapnp
{
	void Encode::encode(lua_State *L, int index, const schema::Struct& s, capnp::_::PointerBuilder& pointer) {
		if (!lua_istable(L, index))
			return;
		capnp::_::StructBuilder p = pointer.initStruct(s.size);
		encodeStruct(L, index, s, p);
	}
	
	void Encode::encodeStruct(lua_State *L, int index, const schema::Struct& s, capnp::_::StructBuilder& p) {
		int idx = index;
		lua_pushnil(L);
		if (idx < 0)
			idx--;
		while (lua_next(L, idx) != 0) {
			if (LUA_TSTRING == lua_type(L, -2)) {
				const char* fname = lua_tostring(L, -2);
				encodeField(L, -1, s, p, fname);
			}
			lua_pop(L, 1);
		}
	}
	
	void Encode::encodeField(lua_State *L, int index, const schema::Struct& s, capnp::_::StructBuilder& p, const char* fname) {
		auto iss = s.slots.find(fname);
		if (iss != s.slots.end()) {
			const schema::SlotField& ss = iss->second;
			encodeSlot(L, -1, ss, p);
			if (ss.inUnion) {
				p.setDataField<uint16_t>(s.discriminantOffset * capnp::ELEMENTS, ss.discriminantValue);
			}
		} else {
			auto isg = s.groups.find(fname);
			if (isg != s.groups.end()) {
				auto& sg = isg->second;
				if (lua_istable(L, -1)) {
					encodeStruct(L, -1, sg.type, p);
					if (sg.inUnion) {
						p.setDataField<uint16_t>(s.discriminantOffset * capnp::ELEMENTS, sg.discriminantValue);
					}
				}
			} else {
				//ignore extra fields
			}
		}
	}
	
	void Encode::encodeSlot(lua_State *L, int index, const schema::SlotField& ss, capnp::_::StructBuilder& p) {
		const schema::Type& st = ss.type;
		if (st.depth == 0) {
			encodeValue(L, index, ss, p);
		} else {
			if (lua_istable(L, index)) {
				capnp::_::PointerBuilder pb = p.getPointerField(ss.offset);
				encodeDepth(L, index, ss, pb, 1);
			}
		}
	}
		
	void Encode::encodeValue(lua_State *L, int index, const schema::SlotField& ss, capnp::_::StructBuilder& p) {
		const schema::Type& st = ss.type;

		#define ENCODE_VALUE(T) \
			case T: { \
				auto value = getValue<T>(L, index, st.enumType); \
				saveField<T>(p, ss.offset, value); \
			} break;

		switch (st.type) {	
			FOR_ALL_TYPES(ENCODE_VALUE)
			case capnp::schema::Type::LIST: {
				//imposible
			} break;
			case capnp::schema::Type::STRUCT: {
				capnp::_::PointerBuilder pb = p.getPointerField(ss.offset);
				encode(L, index, *st.structType, pb);
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
			} break;
		}
		#undef ENCODE_VALUE
	}
		
	void Encode::encodeDepth(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb, int depth) {	
		const schema::Type& st = ss.type;
		if (depth < st.depth) {
			size_t len = lua_rawlen(L, index);
			capnp::_::ListBuilder lb = pb.initList(capnp::ElementSize::POINTER, len);
			for (size_t i = 0; i < len; i++) {
				capnp::_::PointerBuilder lpb = lb.getPointerElement(i);
				lua_rawgeti(L, index, (lua_Integer)i+1);
				if (lua_istable(L, -1)) {
					encodeDepth(L, -1, ss, lpb, depth+1);
				}
				lua_pop(L, 1);
			}
		} else {
			if (ss.ismap) {
				if (ss.type.type == capnp::schema::Type::STRUCT) {
					encodeMap(L, index, ss, pb);
				} else {
					encodeSet(L, index, ss, pb);
				}
			} else {
				encodeList(L, index, ss, pb);
			}
		}
	}
	
	void Encode::encodeList(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb) {
		const schema::Type& st = ss.type;
		size_t len = lua_rawlen(L, index);
	
		#define ENCODE_LIST(T) \
			case T: { \
				capnp::_::ListBuilder lb = pb.initList(CType<T>::Size, len); \
				for (size_t i = 0; i < len; i++) { \
					lua_rawgeti(L, index, (lua_Integer)i+1); \
					CType<T>::Ref value = getValue<T>(L, -1, st.enumType); \
					saveListElement<T>(lb, i, value); \
					lua_pop(L, 1); \
				} \
			} break;

		switch (st.type) {
			FOR_ALL_TYPES(ENCODE_LIST)
			case capnp::schema::Type::LIST: {
				//imposible
			} break;
			case capnp::schema::Type::STRUCT: {
				capnp::_::ListBuilder lb = pb.initStructList(len, st.structType->size);
				for (size_t i = 0; i < len; i++) {
					lua_rawgeti(L, index, (lua_Integer)i+1);
					if (lua_istable(L, -1)) {
						capnp::_::StructBuilder sb = lb.getStructElement(i);
						encodeStruct(L, -1, *st.structType, sb);
					}
					lua_pop(L, 1);
				}
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
			} break;
		}			
		#undef ENCODE_LIST
	}

	
	void Encode::encodeSet(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb) {
		const schema::Type& st = ss.type;

		#define ENCODE_SET(T) \
			case T: { \
				std::set<CType<T>::Copy> keys = getKeys<T>(L, index, st.enumType); \
				size_t len = keys.size(); \
				capnp::_::ListBuilder lb = pb.initList(CType<T>::Size, len); \
				lua_Integer i = 0; \
				for (auto& key : keys) { \
					auto ref = CType<T>::asRef(key); \
					saveListElement<T>(lb, i, ref); \
					i++; \
				} \
			} break;
			
		switch (st.type) {
			FOR_ALL_TYPES(ENCODE_SET)
			case capnp::schema::Type::LIST: {
				//imposible
			} break;
			case capnp::schema::Type::STRUCT: {
				//imposible
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
			} break;
		}
		#undef ENCODE_SET
	}

	void Encode::encodeMap(lua_State *L, int index, const schema::SlotField& ss, capnp::_::PointerBuilder& pb) {
		const schema::Type& st = ss.type;
		const schema::Struct& es = *st.structType;
		auto ki = es.slots.find(ss.mapkey);
		if (ki == es.slots.end()) {
			return;
		}
		const schema::SlotField& kss = ki->second;
		const schema::Type& kst = kss.type;

		#define ENCODE_MAP(T) \
			case T: { \
				std::set<CType<T>::Copy> keys = getKeys<T>(L, index, kst.enumType); \
				size_t len = keys.size(); \
				capnp::_::ListBuilder lb = pb.initStructList(len, es.size); \
				lua_Integer i = 0; \
				for (auto& key : keys) { \
					capnp::_::StructBuilder sb = lb.getStructElement(i); \
					auto ref = CType<T>::asRef(key); \
					pushValue<T>(L, ref, kst.enumType); \
					int idx = index; \
					if (idx < 0) \
						idx -= 1; \
					lua_rawget(L, idx); \
					if (ss.mapvalue.empty()) { \
						encodeStruct(L, -1, *st.structType, sb); \
					} else { \
						encodeField(L, -1, *st.structType, sb, ss.mapvalue.c_str()); \
					} \
					saveField<T>(sb, kss.offset, ref); \
					lua_pop(L, 1); \
					i++; \
				} \
			} break;
		switch (kst.type) {
			FOR_ALL_TYPES(ENCODE_MAP)
			case capnp::schema::Type::LIST: {
				//imposible
			} break;
			case capnp::schema::Type::STRUCT: {
				//imposible
			}
			break;
			case capnp::schema::Type::INTERFACE:
			case capnp::schema::Type::ANY_POINTER: {
				//not supported
			} break;
		}
		#undef ENCODE_MAP
	}
}

