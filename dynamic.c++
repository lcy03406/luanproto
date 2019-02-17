#include "pch.h"
#include "dynamic.h"
#include "schema.h"
#include "encode.h"
#include "decode.h"

namespace luacapnp {
	Dynamic::Dynamic(schema::Registry& registry)
			: registry(registry) {
	}
	
	int Dynamic::interface(lua_State *L) {
		const char* name = luaL_checkstring(L, 1);
		auto* p = registry.findInterface(name);
		if (p == nullptr)
			return 0;
		lua_pushlightuserdata(L, (void*)(p));
		return 1;
	}

	
	int Dynamic::encode(lua_State *L) {
		int ordinal = -1;
		const schema::Struct* schema = findSchema(L, 1, &ordinal);	
		capnp::MallocMessageBuilder alloc; //only for malloc. its internal arena builder is not used.
		capnp::_::BuilderArena arena(&alloc, false);
		auto allocation = arena.allocate(1 * capnp::WORDS);
		capnp::_::SegmentBuilder* segment = allocation.segment;
		capnp::word* rootLocation = allocation.words;
		capnp::_::PointerBuilder root = capnp::_::PointerBuilder::getRoot(segment, nullptr, rootLocation);
		Encode::encode(L, 4, *schema, root);
		kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments = arena.getSegmentsForOutput();
		auto words = capnp::messageToFlatArray(segments);
		auto array = words.asBytes();
		lua_pushinteger(L, ordinal);
		lua_pushlstring(L, (const char*)array.begin(), array.size());
		return 2;
	}
	
	int Dynamic::decode(lua_State *L) {
		kj::StringPtr name;
		auto schema = findSchema(L, 1, nullptr, &name);
		size_t len = 0;
		const char* bytes = luaL_checklstring(L, 4, &len);
		int offset = (int)luaL_checkinteger(L, 5);
		auto words = kj::arrayPtr((const capnp::word*)(bytes+offset), (len-offset)/sizeof(capnp::word));
		lua_pushlstring(L, name.cStr(), name.size());
		//TODO what if bytes are not alined?
		capnp::FlatArrayMessageReader message(words);
		capnp::_::ReaderArena arena(&message);
		auto segment = arena.tryGetSegment(capnp::_::SegmentId(0));
		if (segment == nullptr) {
			return 1;
		}
		const capnp::word* rootLocation = segment->getStartPtr();
		capnp::_::PointerReader root = capnp::_::PointerReader::getRoot(segment, nullptr, rootLocation, 64);
		if (!Decode::decode(L, *schema, root)) {
			return 1;
		}
		return 2;
	}
	
	int Dynamic::pretty(lua_State *L) {
		kj::StringPtr name;
		auto schema = findSchema(L, 1, nullptr, &name);
		auto base = registry.findCapnpStructSchema(schema);
		size_t len = 0;
		const char* bytes = luaL_checklstring(L, 4, &len);
		int offset = (int)luaL_checkinteger(L, 5);
		//TODO what if bytes are not alined?
		auto words = kj::arrayPtr((const capnp::word*)(bytes+offset), (len-offset)/sizeof(capnp::word));
		capnp::FlatArrayMessageReader message(words);
		auto text = kj::str(capnp::prettyPrint(message.getRoot<capnp::DynamicStruct>(base)));
		lua_pushlstring(L, name.cStr(), name.size());
		lua_pushlstring(L, text.cStr(), text.size());
		return 2;
	}
	
	int Dynamic::serialize(lua_State *L) {
		const schema::Struct* schema = findStructSchema(L, 1);	
		capnp::MallocMessageBuilder alloc; //only for malloc. its internal arena builder is not used.
		capnp::_::BuilderArena arena(&alloc, false);
		auto allocation = arena.allocate(1 * capnp::WORDS);
		capnp::_::SegmentBuilder* segment = allocation.segment;
		capnp::word* rootLocation = allocation.words;
		capnp::_::PointerBuilder root = capnp::_::PointerBuilder::getRoot(segment, nullptr, rootLocation);
		Encode::encode(L, 2, *schema, root);
		kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> segments = arena.getSegmentsForOutput();
		auto words = capnp::messageToFlatArray(segments);
		auto array = words.asBytes();
		lua_pushlstring(L, (const char*)array.begin(), array.size());
		return 1;
	}
	
	int Dynamic::deserialize(lua_State *L) {
		const schema::Struct* schema = findStructSchema(L, 1);
		size_t len = 0;
		const char* bytes = luaL_checklstring(L, 2, &len);
		int offset = 0;
		auto words = kj::arrayPtr((const capnp::word*)(bytes+offset), (len-offset)/sizeof(capnp::word));
		//TODO what if bytes are not alined?
		capnp::FlatArrayMessageReader message(words);
		capnp::_::ReaderArena arena(&message);
		auto segment = arena.tryGetSegment(capnp::_::SegmentId(0));
		if (segment == nullptr) {
			return 0;
		}
		const capnp::word* rootLocation = segment->getStartPtr();
		capnp::_::PointerReader root = capnp::_::PointerReader::getRoot(segment, nullptr, rootLocation, 64);
		if (!Decode::decode(L, *schema, root)) {
			return 0;
		}
		return 1;
	}
	
	int Dynamic::text(lua_State *L) {
		auto schema = findStructSchema(L, 1);
		auto base = registry.findCapnpStructSchema(schema);
		size_t len = 0;
		const char* bytes = luaL_checklstring(L, 2, &len);
		int pretty = lua_toboolean(L, 3);
		auto words = kj::arrayPtr((const capnp::word*)bytes, len/sizeof(capnp::word));
		capnp::FlatArrayMessageReader message(words);
		auto text = pretty ? kj::str(capnp::prettyPrint(message.getRoot<capnp::DynamicStruct>(base)))
			: kj::str(message.getRoot<capnp::DynamicStruct>(base));
		lua_pushlstring(L, text.cStr(), text.size());
		return 1;
	}
	
	int Dynamic::parse(lua_State *L) {
		const char* name = luaL_checkstring(L, 1); //TODO multiple structs
		const char* Name = luaL_checkstring(L, 2); //TODO multiple structs
		const char* filename = luaL_checkstring(L, 3);
		const char* import = luaL_checkstring(L, 4); //TODO multiple pathes
		const char* import1 = luaL_checkstring(L, 5); //TODO multiple pathes
		const void* p = registry.parse(name, Name, filename, import, import1);
		lua_pushlightuserdata(L, (void*)p);
		return 1;
	}

	//interface, method, side => schema
	const schema::Struct* Dynamic::findSchema(lua_State *L, int index, int *ordinal, kj::StringPtr* name)
	{
		void* lface = lua_touserdata(L, index); //TODO error handling
		const char* lmethod = nullptr;
		int lidx = 0;
		if (lua_isnumber(L, 2)) {
			lidx = (int)lua_tointeger(L, index + 1);
		} else {
			lmethod = luaL_checkstring(L, index + 1);
		}
		int lside = lua_toboolean(L, index + 2);
		const auto* face = static_cast<const schema::Interface*>(lface);
		const auto* method = (lmethod == nullptr) ? face->getMethod(lidx) : face->getMethod(lmethod);
		if (method == nullptr)
			return nullptr;
		if (ordinal) {
			*ordinal = method->ordinal;
		}
		if (name) {
			*name = method->name;
		}
		return lside ? method->result : method->param;
	}

	//name => schema
	const schema::Struct* Dynamic::findStructSchema(lua_State *L, int index)
	{
		auto lname = luaL_checkstring(L, index);
		auto s = registry.findStruct(lname);
		KJ_ASSERT(s != nullptr, lname);
		return s;
	}
}

