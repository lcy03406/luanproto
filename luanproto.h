#pragma once

namespace luanproto
{
	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, const char* method);

#ifdef LUA_VERSION
	void initSchema(const char* name, capnp::InterfaceSchema schema);
	capnp::StructSchema findSchema(lua_State *L, int index, int *ordinal = nullptr, kj::StringPtr* name = nullptr);

	capnp::Orphan<capnp::DynamicValue> convertToValue(lua_State *L, int index, capnp::MessageBuilder& message, const capnp::Type& type);
	capnp::Orphan<capnp::DynamicValue> convertToList(lua_State *L, int index, capnp::MessageBuilder& message, const capnp::ListSchema& listSchema);
	capnp::Orphan<capnp::DynamicValue> convertToStruct(lua_State *L, int index, capnp::MessageBuilder& message, const capnp::StructSchema& structSchema);

	int convertFromValue(lua_State *L, capnp::DynamicValue::Reader value);
	int convertFromList(lua_State *L, capnp::DynamicList::Reader value);
	int convertFromStruct(lua_State *L, capnp::DynamicStruct::Reader value);
#endif
}
