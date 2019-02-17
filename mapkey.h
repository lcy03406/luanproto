#pragma once
#include "schema.h"

namespace luacapnp
{
	class MapKey : public kj::OneOf<lua_Number, lua_Integer, kj::String> {
	public:
		typedef kj::OneOf<lua_Number, lua_Integer, kj::String> Base;
		MapKey(lua_Number v) : Base(v) {}
		MapKey(lua_Integer v) : Base(v) {}
		MapKey(kj::String&& v) : Base(kj::mv(v)) {}
		bool operator < (const MapKey& other) const {
			auto w1 = this->which();
			auto w2 = other.which();
			if (w1 != w2) {
				return w1 < w2;
			}
			KJ_SWITCH_ONEOF((*this)) {
				KJ_CASE_ONEOF(v, lua_Number) {
					return v < other.get<lua_Number>();
				}
				KJ_CASE_ONEOF(v, lua_Integer) {
					return v < other.get<lua_Integer>();
				}
				KJ_CASE_ONEOF(v, kj::String) {
					return v < other.get<kj::String>();
				}
			}
			return false;
		}
		void push(lua_State *L) const {
			KJ_SWITCH_ONEOF((*this)) {
				KJ_CASE_ONEOF(v, lua_Number) {
					lua_pushnumber(L, v);
				}
				KJ_CASE_ONEOF(v, lua_Integer) {
					lua_pushinteger(L, v);
				}
				KJ_CASE_ONEOF(v, kj::String) {
					lua_pushlstring(L, v.cStr(), v.size());
				}
			}
		}
	};
}
