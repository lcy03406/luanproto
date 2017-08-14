#include "PCH.h"

using namespace capnp;

static int load(lua_State *L) {
	capnp::SchemaParser parser;
	auto schema = parser.parseDiskFile()
	return 0;
}

static int encode (lua_State *L) {
  return 0;
}

static int decode(lua_State *L) {
	return 0;
}



static const luaL_Reg luanprotolib[] = {
	{ "load", load },
	{ "encode", encode },
	{ "decode", decode },
	{NULL, NULL}
};


/*
** Open library
*/
int luaopen_luanproto (lua_State *L) {
  luaL_newlib(L, luanprotolib);
  return 1;
}

