#pragma once

namespace luacapnp
{
	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, const char* method);
	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, int method);
	kj::Maybe<capnp::StructSchema> findStruct(const char* name);
	void initFromFile(const char* filename);
	void initInterfaceSchema(const char* name, capnp::InterfaceSchema schema);
	void initStructSchema(const char* name, capnp::StructSchema schema);

#ifdef LUA_VERSION
	capnp::StructSchema findSchema(lua_State *L, int index, int *ordinal = nullptr, kj::StringPtr* name = nullptr);

	capnp::Orphan<capnp::DynamicValue> convertToValue(lua_State *L, int index, capnp::MessageBuilder& message, const capnp::Type& type, const capnp::StructSchema::Field* field);
	capnp::Orphan<capnp::DynamicValue> convertToList(lua_State *L, int index, capnp::MessageBuilder& message, const capnp::ListSchema& listSchema, const capnp::StructSchema::Field* field);
	capnp::Orphan<capnp::DynamicValue> convertToStruct(lua_State *L, int index, capnp::MessageBuilder& message, const capnp::StructSchema& structSchema);

	int convertFromValue(lua_State *L, capnp::DynamicValue::Reader value, const capnp::StructSchema::Field* field);
	int convertFromList(lua_State *L, capnp::DynamicList::Reader value, const capnp::StructSchema::Field* field);
	int convertFromStruct(lua_State *L, capnp::DynamicStruct::Reader value);
#endif
}
