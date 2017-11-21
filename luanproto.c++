#include <map>

#include <lua/lua.hpp>

#ifdef _MSC_VER
#define KJ_STD_COMPAT
#pragma warning(disable:4996)
#pragma warning(push)
#pragma warning(disable:4244 4267 4800)
#endif
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/pointer-helpers.h>
#include <capnp/dynamic.h>
#include <capnp/schema-parser.h>
#include <capnp/pretty-print.h>
#include <kj/io.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _WIN32
#ifdef LIBLUANP_EXPORTS
#define LIBLUANP_API __declspec(dllexport)
#else
#define LIBLUANP_API __declspec(dllimport)
#endif
#else
#define LIBLUANP_API 
#endif

using namespace capnp;

static capnp::SchemaParser parser;

static std::map<std::string, InterfaceSchema> interfaceSchemaRegistry;

static Orphan<DynamicValue> convertToValue(lua_State *L, int index, MessageBuilder& message, const Type& type);
static Orphan<DynamicValue> convertToList(lua_State *L, int index, MessageBuilder& message, const ListSchema& listSchema);
static Orphan<DynamicValue> convertToStruct(lua_State *L, int index, MessageBuilder& message, const StructSchema& structSchema);

static Orphan<DynamicValue> convertToList(lua_State *L, int index, MessageBuilder& message, const ListSchema& listSchema) {
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
		auto value = convertToValue(L, -1, message, elementType);
		if (value.getType() != DynamicValue::UNKNOWN) {
			list.adopt(i-1, std::move(value));
		}
		lua_pop(L, 1);
	}
	return orphan;
}

static Orphan<DynamicValue> convertToStruct(lua_State *L, int index, MessageBuilder& message, const StructSchema& structSchema) {
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
				auto value = convertToValue(L, -1, message, fieldType);
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

static Orphan<DynamicValue> convertToValue(lua_State *L, int index, MessageBuilder& message, const Type& type) {
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
			return convertToList(L, index, message, listSchema);
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
			return convertToStruct(L, index, message, structSchema);
		} break;
		case schema::Type::INTERFACE:
		case schema::Type::ANY_POINTER: {
			//TODO ?
			return nullptr;
		} break;
	}
	return nullptr;
}


static int convertFromValue(lua_State *L, DynamicValue::Reader value);
static int convertFromList(lua_State *L, DynamicList::Reader value);
static int convertFromStruct(lua_State *L, DynamicStruct::Reader value);

static int convertFromValue(lua_State *L, DynamicValue::Reader value) {
	switch (value.getType()) {
		case DynamicValue::UNKNOWN: {
			return 0;
		} break;
		case DynamicValue::VOID: {
			lua_pushboolean(L, true);
			return 1;
		} break;
		case DynamicValue::BOOL: {
			lua_pushboolean(L, value.as<bool>());
			return 1;
		} break;
		case DynamicValue::INT: {
			lua_pushinteger(L, value.as<int64_t>());
			return 1;
		} break;
		case DynamicValue::UINT: {
			//TODO handle big values
			lua_pushinteger(L, value.as<uint64_t>());
			return 1;
		} break;
		case DynamicValue::FLOAT: {
			lua_pushnumber(L, value.as<double>());
			return 1;
		} break;
		case DynamicValue::TEXT: {
			auto text = value.as<Text>();
			lua_pushlstring(L, text.cStr(), text.size());
			return 1;
		} break;
		case DynamicValue::DATA: {
			auto data = value.as<Data>();
			lua_pushlstring(L, (const char*)data.begin(), data.size());
			return 1;
		} break;
		case DynamicValue::LIST: {
			return convertFromList(L, value.as<DynamicList>());
		} break;
		case DynamicValue::ENUM: {
			auto enumValue = value.as<DynamicEnum>();
			KJ_IF_MAYBE(enumerant, enumValue.getEnumerant()) {
				auto name = enumerant->getProto().getName();
				lua_pushlstring(L, name.cStr(), name.size());
				return 1;
			} else {
				// Unknown enum value; output raw number.
				lua_pushinteger(L, enumValue.getRaw());
				return 1;
			}
		} break;
		case DynamicValue::STRUCT: {
			return convertFromStruct(L, value.as<DynamicStruct>());
		} break;
		case DynamicValue::CAPABILITY:
		case DynamicValue::ANY_POINTER: {
			//TODO ?
			return 0;
		} break;
	}
	return 0;
}

static int convertFromList(lua_State *L, DynamicList::Reader value) {
	lua_newtable(L);
	unsigned size = value.size();
	for (unsigned i = 0; i < size; ++i) {
		auto element = value[i];
		if (convertFromValue(L, element)) {
			lua_rawseti(L, -2, i);
		}
	}
	return 1;
}
static int convertFromStruct(lua_State *L, DynamicStruct::Reader value) {
	lua_newtable(L);
	//TODO(optimize): do not loop through all fields in union
	for (auto field: value.getSchema().getFields()) {
		if (!value.has(field))
			continue;
		auto name = field.getProto().getName();
		lua_pushlstring(L, name.cStr(), name.size());
		if (convertFromValue(L, value.get(field))) {
			lua_rawset(L, -3);
		} else {
			//value is invalid, pop the key
			lua_pop(L, 1);
		}
	}
	return 1;
}


template<typename Func>
static int TryCatch(lua_State *L, Func&& func)
{
	int ret = 0;
	KJ_IF_MAYBE(e, kj::runCatchingExceptions([&]() {
		ret = func();
	})) {
		auto desc = e->getDescription();
		lua_pushlstring(L, desc.cStr(), desc.size());
		return lua_error(L);
	}
	return ret;
}

//interface, method, side => schema
static const StructSchema luaFindSchema(lua_State *L, int *index = nullptr, kj::StringPtr* name = nullptr) {
	void* lface = lua_touserdata(L, 1); //TODO error handling
	const char* lmethod = nullptr;
	int lidx = 0;
	if (lua_isinteger(L, 2)) {
		lidx = lua_tointeger(L, 2);
	} else {
		lmethod = luaL_checkstring(L, 2);
	}
	bool lside = lua_toboolean(L, 3);
	const InterfaceSchema* face = (InterfaceSchema*)lface;
	const auto method = (lmethod == nullptr) ? face->getMethods()[lidx] : face->getMethodByName(lmethod);
	if (index) {
		*index = method.getOrdinal();
	}
	if (name) {
		*name = method.getProto().getName();
	}
	return lside ? method.getResultType() : method.getParamType();	
}


static int lparse(lua_State *L) {
	const char* name = luaL_checkstring(L, 1); //TODO multiple structs
	const char* filename = luaL_checkstring(L, 2);
	const char* import = luaL_checkstring(L, 3); //TODO multiple pathes
	return TryCatch(L, [=]() {
		kj::StringPtr importPathName{import};
		auto importPath = kj::arrayPtr(&importPathName, 1);
		auto fileSchema = parser.parseDiskFile(filename, filename, importPath);
		auto schema = KJ_REQUIRE_NONNULL(fileSchema.findNested(name));
		interfaceSchemaRegistry[name] = schema.asInterface();
		lua_pushlightuserdata(L, &interfaceSchemaRegistry[name]);
		return 1;
	});
}

static int lencode (lua_State *L) {
	return TryCatch(L, [L]() {
		int index = -1;
		auto schema = luaFindSchema(L, &index);
		//TODO(optimize): use luaL_Buffer to avoid copy?
		MallocMessageBuilder message;
		message.adoptRoot(convertToStruct(L, 4, message, schema));
		auto words = messageToFlatArray(message);
		auto array = words.asBytes();
		lua_pushinteger(L, index);
		lua_pushlstring(L, (const char*)array.begin(), array.size());
		return 2;
	});
}

static int ldecode(lua_State *L) {
	size_t len = 0;
	const char* bytes = luaL_checklstring(L, 4, &len);
	int offset = luaL_checkinteger(L, 5);
	return TryCatch(L, [=]() {
		kj::StringPtr name;
		auto schema = luaFindSchema(L, nullptr, &name);
		//TODO what if bytes are not alined?
		auto words = kj::arrayPtr((const word*)(bytes+offset), (len-offset)/sizeof(word));
		FlatArrayMessageReader message(words);
		lua_pushlstring(L, name.cStr(), name.size());
		return 1 + convertFromStruct(L, message.getRoot<DynamicStruct>(schema));
	});
}

static int lpretty(lua_State *L) {
	size_t len = 0;
	const char* bytes = luaL_checklstring(L, 4, &len);
	int offset = luaL_checkinteger(L, 5);
	return TryCatch(L, [=]() {
		kj::StringPtr name;
		auto schema = luaFindSchema(L, nullptr, &name);
		//TODO what if bytes are not alined?
		auto words = kj::arrayPtr((const word*)(bytes+offset), (len-offset)/sizeof(word));
		FlatArrayMessageReader message(words);
		auto text = kj::str(prettyPrint(message.getRoot<DynamicStruct>(schema)));
		lua_pushlstring(L, name.cStr(), name.size());
		lua_pushlstring(L, text.cStr(), text.size());
		return 2;
	});
}
static int linterface(lua_State *L) {
	const char* lface = luaL_checkstring(L, 1);
	return TryCatch(L, [=]() {
		auto it = interfaceSchemaRegistry.find(lface);
		if (it == interfaceSchemaRegistry.end())
			return 0;
		lua_pushlightuserdata(L, &it->second);
		return 1;
	});
}

static const luaL_Reg luanprotolib[] = {
	{ "parse", lparse },
	{ "encode", lencode },
	{ "decode", ldecode },
	{ "pretty", lpretty },
	{ "interface", linterface },
	{NULL, NULL}
};


extern "C" int LIBLUANP_API luaopen_luanproto (lua_State *L) {
  luaL_newlib(L, luanprotolib);
  return 1;
}

