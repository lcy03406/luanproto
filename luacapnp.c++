#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <set>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#ifdef _MSC_VER
#include <io.h>
#define KJ_STD_COMPAT
#pragma warning(disable:4996)
#pragma warning(push)
#pragma warning(disable:4244) // conversion from 'unsigned __int64' to 'capnp::uint', possible loss of data
#pragma warning(disable:4267) // conversion from 'size_t' to 'capnp::uint', possible loss of data
#pragma warning(disable:4521) // multiple copy constructors specified
#pragma warning(disable:4800) // forcing value to bool 'true' or 'false' (performance warning)

#endif
#include <kj/debug.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/pointer-helpers.h>
#include <capnp/dynamic.h>
#include <capnp/schema-parser.h>
#include <capnp/pretty-print.h>
#include <kj/io.h>
#include <kj/one-of.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <lua/lua.hpp>
#if LUA_VERSION_NUM<502
#define lua_rawlen lua_objlen
#endif

#include "luacapnp.h"

#ifdef _WIN32 // TODO add options to static linkage
#define LIBLUACAPNP_API __declspec(dllexport)
#else
#define LIBLUACAPNP_API 
#endif

#define LUA_INT_IS_TOO_SMALL sizeof(lua_Integer)<sizeof(int64_t)

#define LuaTryCatch LuaCapnpTryCatch

template<typename Func>
inline int LuaTryCatch(lua_State *L, Func&& func)
{
	int ret = 0;
	bool jump = false;
	lua_gc(L, LUA_GCSTOP, 0);
	//make sure cb is destructed before longjmp
	//TODO for now, func() must not raise any lua error
	{
		KJ_IF_MAYBE(e, kj::runCatchingExceptions([&]()
		{
			ret = func();
		}))
		{
			auto desc = e->getDescription();
			lua_pushlstring(L, desc.cStr(), desc.size());
			jump = true;
		}
	}
	lua_gc(L, LUA_GCRESTART, 0);
	if (jump)
	{
		return lua_error(L);
	}
	return ret;
}

using namespace capnp;

namespace luacapnp
{
	capnp::Orphan<capnp::DynamicValue> convertToValue(lua_State *L, int index, capnp::MessageBuilder& message, Orphanage& orphanage, const capnp::Type& type, const capnp::StructSchema::Field* field);
	capnp::Orphan<capnp::DynamicValue> convertToList(lua_State *L, int index, capnp::MessageBuilder& message, Orphanage& orphanage, const capnp::ListSchema& listSchema, const capnp::StructSchema::Field* field);
	capnp::Orphan<capnp::DynamicValue> convertToStruct(lua_State *L, int index, capnp::MessageBuilder& message, Orphanage& orphanage, const capnp::StructSchema& structSchema);

	int convertFromValue(lua_State *L, capnp::DynamicValue::Reader value, const capnp::StructSchema::Field* field);
	int convertFromList(lua_State *L, capnp::DynamicList::Reader value, const capnp::StructSchema::Field* field);
	int convertFromStruct(lua_State *L, capnp::DynamicStruct::Reader value);
#ifdef LUACAPNP_PARSER
	capnp::SchemaParser parser;
#endif //LUACAPNP_PARSER
	SchemaLoader loader;

	std::set<kj::String> names;
	std::map<kj::StringPtr, InterfaceSchema> interfaceSchemaRegistry;
	std::map<kj::StringPtr, StructSchema> structSchemaRegistry;

	kj::Maybe<std::pair<kj::StringPtr, kj::StringPtr>> isMap(const StructSchema::Field* field)
	{
		if (field == nullptr)
			return nullptr;
		auto proto = field->getProto();
		if (!proto.hasAnnotations())
			return nullptr;
		bool hasKey = false;
		std::pair<kj::StringPtr, kj::StringPtr> ret;
		for (auto anno : proto.getAnnotations())
		{
			auto id = anno.getId();
			if (id == 0xe3caf55077c0bd92)
			{
				ret.first = anno.getValue().getText();
				hasKey = true;
			}
			else if (id == 0x8e28078f594c7ec5)
			{
				ret.second = anno.getValue().getText();
			}
		}
		if (!hasKey)
			return nullptr;
		return ret;
	}

	Orphan<DynamicValue> convertArrayToList(lua_State *L, int index, MessageBuilder& message, Orphanage& orphanage, const ListSchema& listSchema)
	{
		if (!lua_istable(L, index))
			return nullptr;
		size_t size = lua_rawlen(L, index);
		//if (size == 0)
		//	return nullptr;
		if (index < 0)
			index--;
		auto orphan = orphanage.newOrphan(listSchema, (capnp::uint)size);
		auto list = orphan.get();
		auto elementType = listSchema.getElementType();
		for (size_t i = 1; i <= size; ++i)
		{
			lua_pushinteger(L, i);
			lua_gettable(L, index);
			//TODO(optimize): avoid copy structs
			auto value = convertToValue(L, -1, message, orphanage, elementType, nullptr);
			if (value.getType() != DynamicValue::UNKNOWN)
			{
				list.adopt((capnp::uint)i - 1, kj::mv(value));
			}
			lua_pop(L, 1);
		}
		return kj::mv(orphan);
	}

	class MapKey : public kj::OneOf<lua_Number, lua_Integer, kj::String> {
	public:
		typedef kj::OneOf<lua_Number, lua_Integer, kj::String> Base;
		MapKey(lua_Number v) : Base(v) {}
		MapKey(lua_Integer v) : Base(v) {}
		MapKey(kj::String&& v) : Base(kj::mv(v)) {}
		bool operator < (const MapKey& other) const {
			auto w1 = this->which();
			auto w2 = other.which();
			if (w1 != w2) {
				return w1 < w2;
			}
			KJ_SWITCH_ONEOF((*this)) {
				KJ_CASE_ONEOF(v, lua_Number) {
					return v < other.get<lua_Number>();
				}
				KJ_CASE_ONEOF(v, lua_Integer) {
					return v < other.get<lua_Integer>();
				}
				KJ_CASE_ONEOF(v, kj::String) {
					return v < other.get<kj::String>();
				}
			}
			return false;
		}
		void push(lua_State *L) const {
			KJ_SWITCH_ONEOF((*this)) {
				KJ_CASE_ONEOF(v, lua_Number) {
					lua_pushnumber(L, v);
				}
				KJ_CASE_ONEOF(v, lua_Integer) {
					lua_pushinteger(L, v);
				}
				KJ_CASE_ONEOF(v, kj::String) {
					lua_pushlstring(L, v.cStr(), v.size());
				}
			}
		}
	};

	Orphan<DynamicValue> convertMapToList(lua_State *L, int index, MessageBuilder& message, Orphanage& orphanage, const ListSchema& listSchema, const kj::StringPtr& keyName, const kj::StringPtr& valueName)
	{
		if (!lua_istable(L, index))
			return nullptr;
		std::set<MapKey> keys;
		lua_pushnil(L);
		if (index < 0)
			index--;
		while (lua_next(L, index) != 0) {
			if (lua_isnumber(L, -2)) {
				lua_Integer i = lua_tointeger(L, -2);
				lua_Number f = lua_tonumber(L, -2);
				if (i == (lua_Integer)f && f == (lua_Number)i)
				{
					keys.emplace(i);
				}
				else
				{
					keys.emplace(f);
				}
			} else if (lua_isstring(L, -2)) {
				size_t len;
				const char* str = lua_tolstring(L, -2, &len);
				keys.emplace(kj::heapString(str, len));
			} else {
				//TODO support more types?
			}
			lua_pop(L, 1);
		}
		auto size = keys.size();
		//if (size == 0)
		//	return nullptr;
		if (index < 0)
			index--;
		auto orphan = message.getOrphanage().newOrphan(listSchema, (capnp::uint)size);
		auto list = orphan.get();
		auto elementType = listSchema.getElementType();
		auto keyFieldMaybe = keyName.size() > 0 ? elementType.asStruct().findFieldByName(keyName) : nullptr;
		auto valueFieldMaybe = valueName.size() > 0 ? elementType.asStruct().findFieldByName(valueName) : nullptr;
		KJ_ASSERT((keyName.size() == 0) == (keyFieldMaybe == nullptr));
		KJ_ASSERT((valueName.size() == 0) == (valueFieldMaybe == nullptr));
		capnp::uint i = 0;
		for (auto& key : keys) {
			key.push(L);
			lua_pushvalue(L, -1);
			lua_gettable(L, index);
			KJ_IF_MAYBE(keyField, keyFieldMaybe) {
				KJ_IF_MAYBE(valueField, valueFieldMaybe) {
					auto value = list[i].as<DynamicStruct>();
					auto keyValue = convertToValue(L, -2, message, orphanage, keyField->getType(), keyField);
					if (keyValue.getType() != DynamicValue::UNKNOWN) {
						value.adopt(*keyField, kj::mv(keyValue));
					}
					auto valueValue = convertToValue(L, -1, message, orphanage, valueField->getType(), valueField);
					if (valueValue.getType() != DynamicValue::UNKNOWN) {
						value.adopt(*valueField, kj::mv(valueValue));
					}
				} else {
					//TODO(optimize): avoid copy structs
					auto value = convertToValue(L, -1, message, orphanage, elementType, nullptr);
					if (value.getType() != DynamicValue::UNKNOWN) {
						list.adopt(i, kj::mv(value));
					}
				}
			} else {
				auto value = convertToValue(L, -2, message, orphanage, elementType, nullptr);
				if (value.getType() != DynamicValue::UNKNOWN) {
					list.adopt(i, kj::mv(value));
				}
			}
			i++;
			lua_pop(L, 2);
		}
		return kj::mv(orphan);
	}

	Orphan<DynamicValue> convertToList(lua_State *L, int index, MessageBuilder& message, Orphanage& orphanage, const ListSchema& listSchema, const StructSchema::Field* field)
	{
		KJ_IF_MAYBE(map, isMap(field))
		{
			return convertMapToList(L, index, message, orphanage, listSchema, map->first, map->second);
		}
		else
		{
			return convertArrayToList(L, index, message, orphanage, listSchema);
		}
	}


	Orphan<DynamicValue> convertToStruct(lua_State *L, int index, MessageBuilder& message, Orphanage& orphanage, const StructSchema& structSchema)
	{
		if (!lua_istable(L, index))
			return nullptr;
		lua_pushnil(L);
		if (index < 0)
			index--;
		auto orphan = message.getOrphanage().newOrphan(structSchema);
		auto obj = orphan.get();
		while (lua_next(L, index) != 0)
		{
			if (LUA_TSTRING == lua_type(L, -2))
			{
				const char* fieldName = lua_tostring(L, -2);
				KJ_IF_MAYBE(field, structSchema.findFieldByName(fieldName))
				{
					auto fieldType = field->getType();
					//TODO(optimize): avoid coping group fields.
					auto value = convertToValue(L, -1, message, orphanage, fieldType, field);
					if (value.getType() != DynamicValue::UNKNOWN)
					{
						obj.adopt(*field, kj::mv(value));
					}
				}
				else
				{
					//?
				}
			}
			lua_pop(L, 1);
		}
		return kj::mv(orphan);
	}

	Orphan<DynamicValue> convertToValue(lua_State *L, int index, MessageBuilder& message, Orphanage& orphanage, const Type& type, const StructSchema::Field* field)
	{
		//std::cout << "type:" << type.which() << std::endl;
		switch (type.which())
		{
			case schema::Type::VOID:
			{
				return VOID;
			}
			break;
			case schema::Type::BOOL:
			{
				return 0 != lua_toboolean(L, index);
			}
			break;
			case schema::Type::INT8:
			case schema::Type::INT16:
			case schema::Type::INT32:
			{
				return lua_tointeger(L, index);
			}
			break;
			case schema::Type::UINT8:
			case schema::Type::UINT16:
			{
				return (uint32_t)lua_tointeger(L, index);
			}
			break;
			case schema::Type::UINT32:
			{
				if (LUA_INT_IS_TOO_SMALL)
				{
					// TODO: precision
					return (uint32_t)(int64_t)lua_tonumber(L, index);
				}
				else
				{
					return (uint32_t)lua_tointeger(L, index);
				}
			}
			break;
			case schema::Type::INT64:
			{
				if (LUA_INT_IS_TOO_SMALL)
				{
					// TODO: precision
					return (int64_t)lua_tonumber(L, index);
				}
				else
				{
					return lua_tointeger(L, index);
				}
			}
			break;
			case schema::Type::UINT64:
			{
				if (LUA_INT_IS_TOO_SMALL)
				{
					// TODO: precision
					return (uint64_t)(int64_t)lua_tonumber(L, index);
				}
				else
				{
					return (uint64_t)lua_tointeger(L, index);
				}
			}
			break;
			case schema::Type::FLOAT32:
			case schema::Type::FLOAT64:
			{
				return lua_tonumber(L, index);
			}
			break;
			case schema::Type::TEXT:
			{
				size_t len = 0;
				const char* str = lua_tolstring(L, index, &len);
				auto orphan = message.getOrphanage().newOrphan<Text>((capnp::uint)len);
				memcpy(orphan.get().begin(), str, len);
				return kj::mv(orphan);
			}
			break;
			case schema::Type::DATA:
			{
				size_t len = 0;
				const char* str = lua_tolstring(L, index, &len);
				auto orphan = message.getOrphanage().newOrphan<Data>((capnp::uint)len);
				memcpy(orphan.get().begin(), str, len);
				return kj::mv(orphan);
			}
			break;
			case schema::Type::LIST:
			{
				auto listSchema = type.asList();
				return convertToList(L, index, message, orphanage, listSchema, field);
			}
			break;
			case schema::Type::ENUM:
			{
				auto enumSchema = type.asEnum();
				if (lua_isnumber(L, index))
				{
					return DynamicEnum{ enumSchema, (uint16_t)lua_tointeger(L, index) };
				}
				else
				{
					const char* enumstr = lua_tostring(L, index);
					KJ_IF_MAYBE(enumValue, enumSchema.findEnumerantByName(enumstr))
					{
						return DynamicEnum{ *enumValue };
					}
					else
					{
						return nullptr;
					}
				}
			}
			break;
			case schema::Type::STRUCT:
			{
				auto structSchema = type.asStruct();
				return convertToStruct(L, index, message, orphanage, structSchema);
			}
			break;
			case schema::Type::INTERFACE:
			case schema::Type::ANY_POINTER:
			{
				//TODO ?
				return nullptr;
			}
			break;
		}
		return nullptr;
	}

	int convertFromValue(lua_State *L, DynamicValue::Reader value, const StructSchema::Field* field)
	{
		switch (value.getType())
		{
			case DynamicValue::UNKNOWN:
			{
				return 0;
			} break;
			case DynamicValue::VOID:
			{
				lua_pushboolean(L, true);
				return 1;
			} break;
			case DynamicValue::BOOL:
			{
				lua_pushboolean(L, value.as<bool>());
				return 1;
			} break;
			case DynamicValue::INT:
			{
				auto ft = field ? field->getType().which() : 0;
				auto tmp = value.as<int64_t>();
				if(LUA_INT_IS_TOO_SMALL && ft == schema::Type::INT64) {
					// TODO: precision
					lua_pushnumber(L, (lua_Number)tmp);
				}
				else
				{
					lua_pushinteger(L, (lua_Integer)tmp);
				}
				return 1;
			} break;
			case DynamicValue::UINT:
			{
				auto ft = field ? field->getType().which() : 0;
				auto tmp = value.as<uint64_t>();
				if(LUA_INT_IS_TOO_SMALL && ft == schema::Type::UINT64) {
					// TODO: precision
					lua_pushnumber(L, (lua_Number)tmp);
				}
				else
				{
					// convert to signed
					lua_pushinteger(L, (lua_Integer)tmp);
				}
				return 1;
			} break;
			case DynamicValue::FLOAT:
			{
				lua_pushnumber(L, value.as<double>());
				return 1;
			} break;
			case DynamicValue::TEXT:
			{
				auto text = value.as<Text>();
				lua_pushlstring(L, text.cStr(), text.size());
				return 1;
			} break;
			case DynamicValue::DATA:
			{
				auto data = value.as<Data>();
				lua_pushlstring(L, (const char*)data.begin(), data.size());
				return 1;
			} break;
			case DynamicValue::LIST:
			{
				return convertFromList(L, value.as<DynamicList>(), field);
			} break;
			case DynamicValue::ENUM:
			{
				auto enumValue = value.as<DynamicEnum>();
				KJ_IF_MAYBE(enumerant, enumValue.getEnumerant())
				{
					auto name = enumerant->getProto().getName();
					lua_pushlstring(L, name.cStr(), name.size());
					return 1;
				}
				else
				{
					// Unknown enum value; output raw number.
					lua_pushinteger(L, enumValue.getRaw());
					return 1;
				}
			} break;
			case DynamicValue::STRUCT:
			{
				return convertFromStruct(L, value.as<DynamicStruct>());
			} break;
			case DynamicValue::CAPABILITY:
			case DynamicValue::ANY_POINTER:
			{
				//TODO ?
				return 0;
			} break;
		}
		return 0;
	}

	int convertFromListToArray(lua_State *L, DynamicList::Reader value)
	{
		lua_newtable(L);
		unsigned size = value.size();
		for (unsigned i = 0; i < size; ++i)
		{
			auto element = value[i];
			if (convertFromValue(L, element, nullptr))
			{
				lua_rawseti(L, -2, i+1);
			}
		}
		return 1;
	}

	int convertFromListToMap(lua_State *L, DynamicList::Reader value, const kj::StringPtr& keyName, const kj::StringPtr& valueName)
	{
		auto elementType = value.getSchema().getElementType();
		auto keyFieldMaybe = keyName.size() > 0 ? elementType.asStruct().findFieldByName(keyName) : nullptr;
		auto valueFieldMaybe = valueName.size() > 0 ? elementType.asStruct().findFieldByName(valueName) : nullptr;
		lua_newtable(L);
		unsigned size = value.size();
		for (unsigned i = 0; i < size; ++i) {
			auto element = value[i];
			KJ_IF_MAYBE(keyField, keyFieldMaybe) {
				auto elementStruct = element.as<DynamicStruct>();
				auto keyValue = elementStruct.get(*keyField);
				if (convertFromValue(L, keyValue, keyField)) {
					KJ_IF_MAYBE(valueField, valueFieldMaybe) {
						auto valueValue = elementStruct.get(*valueField);
						if (convertFromValue(L, valueValue, valueField)) {
							lua_rawset(L, -3);
						} else {
							lua_pop(L, 1);
						}
					} else {
						if (convertFromValue(L, element, nullptr)) {
							lua_rawset(L, -3);
						} else {
							lua_pop(L, 1);
						}
					}
				}
			} else {
				if (convertFromValue(L, element, nullptr)) {
					lua_pushboolean(L, 1);
					lua_rawset(L, -3);
				}
			}
		}
		return 1;
	}

	int convertFromList(lua_State *L, DynamicList::Reader value, const StructSchema::Field* field)
	{
		KJ_IF_MAYBE(map, isMap(field))
		{
			return convertFromListToMap(L, value, map->first, map->second);
		}
		else
		{
			return convertFromListToArray(L, value);
		}
	}

	int convertFromStruct(lua_State *L, DynamicStruct::Reader value)
	{
		lua_newtable(L);
		//TODO(optimize): do not loop through all fields in union
		for (auto field : value.getSchema().getFields())
		{
			if (!value.has(field))
				continue;
			auto name = field.getProto().getName();
			lua_pushlstring(L, name.cStr(), name.size());
			if (convertFromValue(L, value.get(field), &field))
			{
				lua_rawset(L, -3);
			}
			else
			{
				//value is invalid, pop the key
				lua_pop(L, 1);
			}
		}
		return 1;
	}

	/*
	static kj::StringPtr ShortName(const kj::StringPtr& name)
	{
		KJ_IF_MAYBE(col, name.findLast(':'))
		{
			return name.slice(*col+1);
		}
		else
		{
			return name;
		}
	}
	*/

	void initFromFile(const char* filename)
	{
#ifdef _MSC_VER
		int fd = open(filename, O_RDONLY | O_BINARY);
#else
		int fd = open(filename, O_RDONLY);
#endif
		KJ_ASSERT(fd >= 0, "cannot open file", filename, errno);
		::capnp::PackedFdMessageReader file{ kj::AutoCloseFd(fd) };
		auto root = file.getRoot<schema::CodeGeneratorRequest>();
		auto nodes = root.getNodes();
		kj::Vector<uint64_t> fileids;
		for (auto n : nodes)
		{
			loader.load(n);
		}
		for (auto r : root.getRequestedFiles())
		{
			auto fid = r.getId();
			auto f = loader.get(fid);
			for (auto nn : f.getProto().getNestedNodes())
			{
				auto node = loader.get(nn.getId());
				auto name = nn.getName();
				auto s = node.getProto();
				if (s.isInterface())
				{
					auto lname = kj::str((char)tolower(name[0]), name.slice(1));
					initInterfaceSchema(lname.cStr(), node.asInterface());
				}
				else if (s.isStruct())
				{
					initStructSchema(name.cStr(), node.asStruct());
				}
			}
		}
	}

	void initInterfaceSchema(const char* name, InterfaceSchema schema)
	{
		auto it = names.emplace(kj::str(name)).first;
		interfaceSchemaRegistry[*it] = schema;
	}
	
	void initStructSchema(const char* name, StructSchema schema)
	{
		auto it = names.emplace(kj::str(name)).first;
		structSchemaRegistry[*it] = schema;
	}

	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, const char* method)
	{
		auto it = interfaceSchemaRegistry.find(interface);
		if (it == interfaceSchemaRegistry.end())
			return nullptr;
		return it->second.getMethodByName(method);
	}

	kj::Maybe<capnp::InterfaceSchema::Method> findMethod(const char* interface, int method)
	{
		auto it = interfaceSchemaRegistry.find(interface);
		if (it == interfaceSchemaRegistry.end())
			return nullptr;
		return it->second.getMethods()[method];
	}

	kj::Maybe<capnp::StructSchema> findStruct(const char* name)
	{
		auto it = structSchemaRegistry.find(name);
		if (it == structSchemaRegistry.end())
			return nullptr;
		return it->second;
	}

	//interface, method, side => schema
	StructSchema findSchema(lua_State *L, int index, int *ordinal, kj::StringPtr* name)
	{
		void* lface = lua_touserdata(L, index); //TODO error handling
		const char* lmethod = nullptr;
		int lidx = 0;
		if (lua_isnumber(L, 2))
		{
			lidx = (int)lua_tointeger(L, index + 1);
		}
		else
		{
			lmethod = luaL_checkstring(L, index + 1);
		}
		int lside = lua_toboolean(L, index + 2);
		const InterfaceSchema* face = (InterfaceSchema*)lface;
		const auto method = (lmethod == nullptr) ? face->getMethods()[lidx] : face->getMethodByName(lmethod);
		if (ordinal)
		{
			*ordinal = method.getOrdinal();
		}
		if (name)
		{
			*name = method.getProto().getName();
		}
		return lside ? method.getResultType() : method.getParamType();
	}

	//name => schema
	StructSchema findStructSchema(lua_State *L, int index)
	{
		auto lname = luaL_checkstring(L, index);
		auto it = structSchemaRegistry.find(lname);
		KJ_ASSERT(it != structSchemaRegistry.end());
		return it->second;
	}
}

using namespace luacapnp;

#ifdef LUACAPNP_PARSER
static int lparse(lua_State *L) {
	const char* name = luaL_checkstring(L, 1); //TODO multiple structs
	const char* Name = luaL_checkstring(L, 2); //TODO multiple structs
	const char* filename = luaL_checkstring(L, 3);
	const char* import = luaL_checkstring(L, 4); //TODO multiple pathes
	const char* import1 = luaL_checkstring(L, 5); //TODO multiple pathes
	return LuaTryCatch(L, [=]() {
		kj::StringPtr importPathName[2] = {import, import1};
		auto importPath = kj::arrayPtr(importPathName, 2);
		auto fileSchema = parser.parseDiskFile(filename, filename, importPath);
		auto schema = KJ_REQUIRE_NONNULL(fileSchema.findNested(Name));
		if (schema.getProto().isInterface()) {
			auto& p = interfaceSchemaRegistry[name] = schema.asInterface();
			lua_pushlightuserdata(L, &p);
			return 1;
		} else if (schema.getProto().isStruct()) {
			auto& p = structSchemaRegistry[Name] = schema.asStruct();
			lua_pushlightuserdata(L, &p);
			return 1;
		}
		return 0;
	});
}
#endif //LUACAPNP_PARSER


static int linterface(lua_State *L) {
	const char* lface = luaL_checkstring(L, 1);
	return LuaTryCatch(L, [=]() {
		auto it = interfaceSchemaRegistry.find(lface);
		if (it == interfaceSchemaRegistry.end())
			return 0;
		lua_pushlightuserdata(L, &it->second);
		return 1;
	});
}

static int lencode (lua_State *L) {
	return LuaTryCatch(L, [L]() {
		int index = -1;
		auto schema = findSchema(L, 1, &index);
		//TODO(optimize): use luaL_Buffer to avoid copy?
		MallocMessageBuilder message;
		auto orphanage = message.getOrphanage();
		message.adoptRoot(convertToStruct(L, 4, message, orphanage, schema));
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
	int offset = (int)luaL_checkinteger(L, 5);
	return LuaTryCatch(L, [=]() {
		kj::StringPtr name;
		auto schema = findSchema(L, 1, nullptr, &name);
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
	int offset = (int)luaL_checkinteger(L, 5);
	return LuaTryCatch(L, [=]() {
		kj::StringPtr name;
		auto schema = findSchema(L, 1, nullptr, &name);
		//TODO what if bytes are not alined?
		auto words = kj::arrayPtr((const word*)(bytes+offset), (len-offset)/sizeof(word));
		FlatArrayMessageReader message(words);
		auto text = kj::str(prettyPrint(message.getRoot<DynamicStruct>(schema)));
		lua_pushlstring(L, name.cStr(), name.size());
		lua_pushlstring(L, text.cStr(), text.size());
		return 2;
	});
}

static int lserialize(lua_State *L) {
	return LuaTryCatch(L, [L]() {
		auto schema = findStructSchema(L, 1);
		//TODO(optimize): use luaL_Buffer to avoid copy?
		MallocMessageBuilder message;
		auto orphanage = message.getOrphanage();
		message.adoptRoot(convertToStruct(L, 2, message, orphanage, schema));
		auto words = messageToFlatArray(message);
		auto array = words.asBytes();
		lua_pushlstring(L, (const char*)array.begin(), array.size());
		return 1;
	});
}

static int ldeserialize(lua_State *L) {
	size_t len = 0;
	const char* bytes = luaL_checklstring(L, 2, &len);
	return LuaTryCatch(L, [=]() {
		auto schema = findStructSchema(L, 1);
		auto words = kj::arrayPtr((const word*)bytes, len/sizeof(word));
		ReaderOptions options;
		options.nestingLimit = 1024;
		FlatArrayMessageReader message(words, options);
		return convertFromStruct(L, message.getRoot<DynamicStruct>(schema));
	});
}

static int ltext(lua_State *L) {
	size_t len = 0;
	const char* bytes = luaL_checklstring(L, 2, &len);
	int pretty = lua_toboolean(L, 3);
	return LuaTryCatch(L, [=]() {
		auto schema = findStructSchema(L, 1);
		auto words = kj::arrayPtr((const word*)bytes, len/sizeof(word));
		FlatArrayMessageReader message(words);
		auto text = pretty ? kj::str(prettyPrint(message.getRoot<DynamicStruct>(schema)))
			: kj::str(message.getRoot<DynamicStruct>(schema));
		lua_pushlstring(L, text.cStr(), text.size());
		return 1;
	});
}

#ifdef LUACAPNP_PARSER
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
#ifdef LUACAPNP_PARSER
	{ "parse", lparse },
	{ "enum", lenum },
#endif //LUACAPNP_PARSER
	{ "interface", linterface },
	{ "encode", lencode },
	{ "decode", ldecode },
	{ "pretty", lpretty },
	{ "serialize", lserialize },
	{ "deserialize", ldeserialize },
	{ "text", ltext },
	{NULL, NULL}
};


extern "C" int LIBLUACAPNP_API luaopen_luacapnp (lua_State *L) {
  luaL_newlib(L, luacapnplib);
  return 1;
}

