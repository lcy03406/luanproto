#pragma once
#include "schema.h"
#include "ctype.h"

namespace luacapnp
{
	//getValue
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Ref>
	inline C getValue(lua_State *L, int index, const schema::Enum*) {
		int isnum = 0;
		lua_Integer value = lua_tointegerx(L, index, &isnum);
		if (isnum) {
			return (C) value;
		} else {
			return (C) lua_tonumber(L, index);
		}
	}

	template <>
	inline bool getValue<capnp::schema::Type::VOID>(
			lua_State *L, int index, const schema::Enum*) {
		return true;
	}

	template <>
	inline bool getValue<capnp::schema::Type::BOOL>(
			lua_State *L, int index, const schema::Enum*) {
		return (bool) lua_toboolean(L, index);
	}
	/*
	template <>
	inline int64_t getValue<capnp::schema::Type::INT64>(
			lua_State *L, int index, const schema::Enum*) {
		if (LUA_INT_IS_TOO_SMALL) {
			return (int64_t) lua_tonumber(L, index);
		} else {
			return (int64_t) lua_tointeger(L, index);
		}
	}

	template <>
	inline int64_t getValue<capnp::schema::Type::UINT64>(
			lua_State *L, int index, const schema::Enum*) {
		if (LUA_INT_IS_TOO_SMALL) {
			return (uint64_t) (int64_t) lua_tonumber(L, index);
		} else {
			return (uint64_t) lua_tointeger(L, index);
		}
	}*/
	
	template <>
	inline float getValue<capnp::schema::Type::FLOAT32>(
			lua_State *L, int index, const schema::Enum*) {
		return (float) lua_tonumber(L, index);
	}
	
	template <>
	inline double getValue<capnp::schema::Type::FLOAT64>(
			lua_State *L, int index, const schema::Enum*) {
		return (double) lua_tonumber(L, index);
	}
	
	template <>
	inline kj::ArrayPtr<const char> getValue<capnp::schema::Type::TEXT>(
			lua_State *L, int index, const schema::Enum*) {
		size_t slen = 0;
		const char* str = lua_tolstring(L, index, &slen);
		return kj::ArrayPtr<const char>(str, slen);
	}
	
	template <>
	inline kj::ArrayPtr<const kj::byte> getValue<capnp::schema::Type::DATA>(
			lua_State *L, int index, const schema::Enum*) {
		size_t slen = 0;
		const char* str = lua_tolstring(L, index, &slen);
		return kj::ArrayPtr<const kj::byte>((const kj::byte*)str, slen);
	}

	template <>
	inline uint16_t getValue<capnp::schema::Type::ENUM>(
			lua_State *L, int index, const schema::Enum* et) {
		if (lua_isnumber(L, index)) {
			return (uint16_t)lua_tointeger(L, index);
		} else {
			const char* enumstr = lua_tostring(L, index);
			return (uint16_t)et->tointeger(enumstr);
		}
	}
	
	//getCopy
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Copy>
	inline C getCopy(lua_State *L, int index, const schema::Enum* et) {
		return getValue<T>(L, index, et);
	}
	
	template <>
	inline std::string getCopy<capnp::schema::Type::TEXT>(
			lua_State *L, int index, const schema::Enum*) {
		size_t slen = 0;
		const char* str = lua_tolstring(L, index, &slen);
		return std::string(str, slen);
	}

	template <>
	inline std::basic_string<kj::byte> getCopy<capnp::schema::Type::DATA>(
			lua_State *L, int index, const schema::Enum*) {
		size_t slen = 0;
		const char* str = lua_tolstring(L, index, &slen);
		return std::basic_string<kj::byte>((const kj::byte*)str, slen);
	}
	
	//pushValue
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Ref>
	inline void pushValue(lua_State *L, C value, const schema::Enum*) {
		lua_pushinteger(L, value);
	}

	template <>
	inline void pushValue<capnp::schema::Type::VOID>(
			lua_State *L, bool value, const schema::Enum*) {
		(void)value;
		lua_pushboolean(L, true);
	}

	template <>
	inline void pushValue<capnp::schema::Type::BOOL>(
			lua_State *L, bool value, const schema::Enum*) {
		lua_pushboolean(L, value);
	}

	template <>
	inline void pushValue<capnp::schema::Type::INT64>(
			lua_State *L, int64_t value, const schema::Enum*) {
		if (LUA_INT_IS_TOO_SMALL && (value < LUA_MININTEGER || value > LUA_MAXINTEGER)) {
			lua_pushnumber(L, value);
		} else {
			lua_pushinteger(L, value);
		}
	}

	template <>
	inline void pushValue<capnp::schema::Type::UINT64>(
			lua_State *L, uint64_t value, const schema::Enum*) {
		if (LUA_INT_IS_TOO_SMALL && (value < LUA_MININTEGER || value > LUA_MAXINTEGER)) {
			lua_pushnumber(L, value);
		} else {
			lua_pushinteger(L, value);
		}
	}
	
	template <>
	inline void pushValue<capnp::schema::Type::FLOAT32>(
			lua_State *L, float value, const schema::Enum*) {
		lua_pushnumber(L, value);
	}
	
	template <>
	inline void pushValue<capnp::schema::Type::FLOAT64>(
			lua_State *L, double value, const schema::Enum*) {
		lua_pushnumber(L, value);
	}
	
	template <>
	inline void pushValue<capnp::schema::Type::TEXT>(
			lua_State *L, kj::ArrayPtr<const char> value, const schema::Enum*) {
		if (value.begin() == nullptr) {
			lua_pushnil(L);
		} else {
			lua_pushlstring(L, value.begin(), value.size());
		}
	}
	
	template <>
	inline void pushValue<capnp::schema::Type::DATA>(
			lua_State *L, kj::ArrayPtr<const kj::byte> value, const schema::Enum*) {
		if (value.begin() == nullptr) {
			lua_pushnil(L);
		} else {
			lua_pushlstring(L, (const char*)value.begin(), value.size());
		}
	}
	
	template <>
	inline void pushValue<capnp::schema::Type::ENUM>(
			lua_State *L, uint16_t value, const schema::Enum* et) {
		if (value < et->enumerants.size()) {
			auto& name = et->enumerants[value];
			lua_pushlstring(L, name.c_str(), name.size());
		} else {
			lua_pushinteger(L, value);
		}
	}
	
	//getKeys
	template <capnp::schema::Type::Which T>
	inline std::set<typename CType<T>::Copy> getKeys(
			lua_State *L, int index, const schema::Enum* et) {
		std::set<typename CType<T>::Copy> keys;
		lua_pushnil(L);
		int idx = index;
		if (idx < 0)
			idx--;
		while (lua_next(L, idx) != 0) {
			keys.emplace(getCopy<T>(L, -2, et));
			lua_pop(L, 1);
		}
		return keys;
	}
	
}

