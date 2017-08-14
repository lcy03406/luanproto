#pragma once

class SchemaRegistry
{
	std::map<kj::String, capnp::ParsedSchema> registry;
public:
	bool load();
	const capnp::ParsedSchema* find();
};