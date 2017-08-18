#include "PCH.h"

using namespace capnp;

capnp::SchemaParser parser;

std::map<std::string, StructSchema> schemaRegistry;

static int load(lua_State *L) {
	int n = lua_gettop(L);
	if (n < 3) {
		lua_pushstring(L, "bad arguments, need 3: struct name, filename, import path");
		lua_pushboolean(L, false);
		return 2;
	}
	kj::StringPtr structName = luaL_checkstring(L, 1);
	kj::StringPtr filename = luaL_checkstring(L, 2);
	kj::StringPtr importPathName = luaL_checkstring(L, 3); //TODO multiple pathes
	auto importPath = kj::arrayPtr(&importPathName, 1);
	auto fileSchema = parser.parseDiskFile(filename, filename, importPath);
	KJ_IF_MAYBE(schema, fileSchema.findNested(structName)) {
		schemaRegistry[structName] = schema->asStruct();
		lua_pushboolean(L, true);
		return 1;
	} else {
		luaL_Buffer b;
		luaL_buffinit(L, &b);
		luaL_addstring(&b, "cannot find struct (");
		luaL_addstring(&b, structName.cStr());
		luaL_addstring(&b, ") in file: ");
		luaL_addstring(&b, filename.cStr());
		luaL_pushresult(&b);
		lua_pushboolean(L, false);
		return 2;
	}
}

static Orphan<DynamicValue> readLuaValue(lua_State *L, int index, MessageBuilder& message, const Type& type);
static Orphan<DynamicValue> readLuaList(lua_State *L, int index, MessageBuilder& message, const ListSchema& listSchema);
static Orphan<DynamicValue> readLuaStruct(lua_State *L, int index, MessageBuilder& message, const StructSchema& structSchema);

static Orphan<DynamicValue> readLuaList(lua_State *L, int index, MessageBuilder& message, const ListSchema& listSchema) {
	if (!lua_istable(L, index))
		return nullptr;
	size_t size = lua_rawlen(L, index);
	if (size == 0)
		return nullptr;
	if (index < 0)
		index--;
	auto orphan = message.getOrphanage().newOrphan(listSchema, size);
	auto list = orphan.get();
	auto elementType = listSchema.getElementType();
	for (size_t i = 1; i <= size; ++i) {
		lua_pushinteger(L, i);
		lua_gettable(L, index);
		//TODO(optimize): avoid copy structs
		auto value = readLuaValue(L, -1, message, elementType);
		if (value.getType() != DynamicValue::UNKNOWN) {
			list.adopt(i-1, std::move(value));
		}
		lua_pop(L, 1);
	}
	return orphan;
}

static Orphan<DynamicValue> readLuaStruct(lua_State *L, int index, MessageBuilder& message, const StructSchema& structSchema) {
	if (!lua_istable(L, index))
		return nullptr;
	lua_pushnil(L);
	if (index < 0)
		index--;
	auto orphan = message.getOrphanage().newOrphan(structSchema);
	auto obj = orphan.get();
	while (lua_next(L, index) != 0) {
		if (lua_isstring(L, -2)) {
			const char* fieldName = lua_tostring(L, -2);
			KJ_IF_MAYBE(field, structSchema.findFieldByName(fieldName)) {
				auto fieldType = field->getType();
				//TODO(optimize): avoid coping group fields.
				auto value = readLuaValue(L, -1, message, fieldType);
				if (value.getType() != DynamicValue::UNKNOWN) {
					obj.adopt(*field, std::move(value));
				}
			} else {
				//?
			}
		}
		lua_pop(L, 1);
	}
	return orphan;
}

static Orphan<DynamicValue> readLuaValue(lua_State *L, int index, MessageBuilder& message, const Type& type) {
	switch (type.which()) {
		case schema::Type::VOID: {
			return VOID;
		} break;
		case schema::Type::BOOL: {
			return 0 != lua_toboolean(L, index);
		} break;
		case schema::Type::INT8:
		case schema::Type::INT16:
		case schema::Type::INT32:
		case schema::Type::INT64:
		case schema::Type::UINT8:
		case schema::Type::UINT16:
		case schema::Type::UINT32: {
			return lua_tointeger(L, index);
		} break;
		case schema::Type::UINT64: {
			//TODO handle big values
			return lua_tointeger(L, index);
		} break;
		case schema::Type::FLOAT32:
		case schema::Type::FLOAT64: {
			return lua_tonumber(L, index);
		} break;
		case schema::Type::TEXT: {
			size_t len = 0;
			const char* str = lua_tolstring(L, index, &len);
			auto orphan = message.getOrphanage().newOrphan<Text>(len);
			std::copy(str, str+len, orphan.get().begin());
			return orphan;
		} break;
		case schema::Type::DATA: {
			size_t len = 0;
			const char* str = lua_tolstring(L, index, &len);
			auto orphan = message.getOrphanage().newOrphan<Data>(len);
			std::copy(str, str+len, orphan.get().begin());
			return orphan;
		} break;
		case schema::Type::LIST: {
			auto listSchema = type.asList();
			return readLuaList(L, index, message, listSchema);
		} break;
		case schema::Type::ENUM: {
			auto enumSchema = type.asEnum();
			if (lua_isnumber(L, index)) {
				return DynamicEnum{enumSchema, (uint16_t)lua_tointeger(L, index)};
			} else {
				const char* enumstr = lua_tostring(L, index);
				KJ_IF_MAYBE(enumValue, enumSchema.findEnumerantByName(enumstr)) {
					return DynamicEnum{*enumValue};
				} else {
					return nullptr;
				}
			}
		} break;
		case schema::Type::STRUCT: {
			auto structSchema = type.asStruct();
			return readLuaStruct(L, index, message, structSchema);
		} break;
		case schema::Type::INTERFACE:
		case schema::Type::ANY_POINTER: {
			//TODO ?
			return nullptr;
		} break;
	}
	return nullptr;
}

static int encode (lua_State *L) {
	const char* structName = luaL_checkstring(L, 1);
	const auto it = schemaRegistry.find(structName);
	if (it == schemaRegistry.end())
		return 0;
	const auto& structSchema = it->second;
	MallocMessageBuilder message;
	message.adoptRoot(readLuaStruct(L, 2, message, structSchema));
	//TODO(optimize): use luaL_Buffer to avoid copy?
	kj::VectorOutputStream stream;
	writeMessage(stream, message);
	auto array = stream.getArray();
	lua_pushlstring(L, (const char*)array.begin(), array.size());
	auto text = kj::str(prettyPrint(message.getRoot<DynamicStruct>(structSchema)), '\n');
	lua_pushlstring(L, text.cStr(), text.size());
	return 2;
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
extern "C" int LIBLUANP_API luaopen_luanproto (lua_State *L) {
  luaL_newlib(L, luanprotolib);
  return 1;
}

