#include "pch.h"
#include "luacapnp.h"
#include "schema.h"

namespace luacapnp
{
	capnp::SchemaLoader loader;

	std::set<kj::String> names;
	std::map<kj::StringPtr, capnp::InterfaceSchema> interfaceSchemaRegistry;
	std::map<kj::StringPtr, capnp::StructSchema> structSchemaRegistry;


	void initInterfaceSchema(const char* name, capnp::InterfaceSchema schema)
	{
		auto it = names.emplace(kj::str(name)).first;
		interfaceSchemaRegistry[*it] = schema;
		schema::Registry& reg = schema::Registry::instance();
		reg.load(name, schema);
	}
	
	void initStructSchema(const char* name, capnp::StructSchema schema)
	{
		auto it = names.emplace(kj::str(name)).first;
		structSchemaRegistry[*it] = schema;
		schema::Registry& reg = schema::Registry::instance();
		reg.load(name, schema);
	}

	void initFromFile(const char* filename) {
#ifdef _MSC_VER
		int fd = open(filename, O_RDONLY | O_BINARY);
#else
		int fd = open(filename, O_RDONLY);
#endif
		KJ_ASSERT(fd >= 0, "cannot open file", filename, errno);
		::capnp::PackedFdMessageReader file{ kj::AutoCloseFd(fd) };
		auto root = file.getRoot<capnp::schema::CodeGeneratorRequest>();
		auto nodes = root.getNodes();
		kj::Vector<uint64_t> fileids;
		for (auto n : nodes) {
			loader.load(n);
		}
		for (auto r : root.getRequestedFiles()) {
			auto fid = r.getId();
			auto f = loader.get(fid);
			for (auto nn : f.getProto().getNestedNodes()) {
				auto node = loader.get(nn.getId());
				auto name = nn.getName();
				auto s = node.getProto();
				if (s.isInterface()) {
					auto lname = kj::str((char)tolower(name[0]), name.slice(1));
					initInterfaceSchema(lname.cStr(), node.asInterface());
				} else if (s.isStruct()) {
					initStructSchema(name.cStr(), node.asStruct());
				}
			}
		}
	}

	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, const char* method)
	{
		auto it = interfaceSchemaRegistry.find(interface);
		if (it == interfaceSchemaRegistry.end())
			return nullptr;
		return it->second.getMethodByName(method);
	}

	kj::Maybe<capnp::StructSchema> findStruct(const char* name)
	{
		auto it = structSchemaRegistry.find(name);
		if (it == structSchemaRegistry.end())
			return nullptr;
		return it->second;
	}
}

