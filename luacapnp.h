#pragma once

namespace luacapnp
{
	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, const char* method);
	kj::Maybe<capnp::StructSchema> findStruct(const char* name);
	void initFromFile(const char* filename);
}
