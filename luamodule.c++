#include "pch.h"
#include "luacapnp.h"
#include "dynamic.h"
#include "schema.h"


using namespace luacapnp;

static Dynamic impl(schema::Registry::instance());

static int linterface(lua_State *L) {
	return LuaTryCatch(L, [=]() {
		return impl.interface(L);
	});
}

static int lencode(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.encode(L);
	});
}

static int ldecode(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.decode(L);
	});
}

static int lpretty(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.pretty(L);
	});
}

static int lserialize(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.serialize(L);
	});
}

static int ldeserialize(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.deserialize(L);
	});
}

static int ltext(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.text(L);
	});
}

#ifdef LUACAPNP_PARSER
static int lparse(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		return impl.parse(L);
	});
}

static int lenum(lua_State *L) {
	const char* filepath = luaL_checkstring(L, 1);
	const char* enumname = luaL_checkstring(L, 2);
	const char* rpcdir = luaL_checkstring(L, 3);
	const char* incdir = luaL_checkstring(L, 4);
	auto func = [=] () -> int {
		lua_newtable(L);
		const kj::StringPtr import[] = { rpcdir, incdir };
		auto importPath = kj::arrayPtr(import, kj::size(import));
		capnp::SchemaParser parser;
		auto fileSchema = parser.parseDiskFile(filepath, filepath, importPath);
		auto parsSchema = fileSchema.findNested(enumname);
		if(parsSchema == nullptr) 
		{
			std::cerr << "Parse failed: " <<"not found " << enumname << std::endl; 
			return 1;
		}
		auto schema = ::kj::_::readMaybe(parsSchema);
		if(schema && schema->getProto().isEnum())
		{
			auto enumSchema = schema->asEnum();
			auto enumerants = enumSchema.getEnumerants();
			for(auto enumerant: enumerants)
			{
				auto name = enumerant.getProto().getName();
				lua_pushlstring(L, name.cStr(), name.size());
				lua_pushnumber(L, enumerant.getIndex());
				lua_rawset(L, -3);
			}
		}
		return 1;
	};
	return LuaTryCatch(L, func);
}
#endif

static const luaL_Reg luacapnplib[] = {
	{ "interface", linterface },
	{ "encode", lencode },
	{ "decode", ldecode },
	{ "pretty", lpretty },
	{ "serialize", lserialize },
	{ "deserialize", ldeserialize },
	{ "text", ltext },
#ifdef LUACAPNP_PARSER
	{ "parse", lparse },
	{ "enum", lenum },
#endif //LUACAPNP_PARSER
	{NULL, NULL}
};


extern "C" int LIBLUACAPNP_API luaopen_luacapnp (lua_State *L) {
  luaL_newlib(L, luacapnplib);
  return 1;
}

