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

#endif
}
