#pragma once
namespace luacapnp {
	typedef std::string String;
	typedef const char * StringPtr;
	namespace schema {
		struct Struct;
		struct Enum;
		struct Interface;
		struct Field;
		struct SlotField;
		struct GroupField;
		struct Method;
		struct Type;
		struct PrimType;
		struct ListType;
		struct StructType;
		class RegistryData;
		class Registry;


		struct Type {
			using CapnpType = capnp::schema::Type::Which;
			CapnpType type = CapnpType::VOID;
			int depth = 0;
			union {
				const Enum* enumType;
				const Struct* structType;
			};

			void load(RegistryData* regdata, const capnp::Type& t);
		};

		struct Struct {
			capnp::_::StructSize size;
			bool isGroup = false;
			bool hasUnion = false;
			unsigned discriminantOffset = 0;

			std::map<std::string, SlotField> slots;
			std::map<std::string, GroupField> groups;

			void load(RegistryData* regdata, const capnp::StructSchema& s);
		};

		struct Enum {
			std::vector<String> enumerants;
			std::map<String, unsigned> index;

			void load(RegistryData* regdata, const capnp::EnumSchema& s);
			unsigned tointeger(StringPtr name) const;
		};

		struct Interface {
			std::vector<Method> methods;
			std::map<std::string, Method*> index;

			void load(RegistryData* regdata, const capnp::InterfaceSchema& s);
			const Method* getMethod(unsigned ordinal) const;
			const Method* getMethod(StringPtr name) const;
		};

		struct Field {
			bool inUnion;
			unsigned discriminantValue;
		};

		struct SlotField : public Field {
			unsigned offset;
			Type type;

			bool ismap = false;
			String mapkey;
			String mapvalue;

			void load(RegistryData* regdata, const capnp::schema::Field::Reader& fp);
		};

		struct GroupField : public Field {
			Struct type;
		};

		struct Method {
			String name;
			uint16_t ordinal;
			const Struct* param;
			const Struct* result;
		};

		class RegistryData {
			typedef uint64_t Id;
			std::map<Id, Interface> interfaces;
			std::map<Id, Struct> structs;
			std::map<Id, Enum> enums;
		public:
			const Enum* load(const capnp::EnumSchema& s);
			const Struct* load(const capnp::StructSchema& s);
			const Interface* load(const capnp::InterfaceSchema& s);
		};

		class Registry {
			RegistryData data;
			std::map<String, const Struct*> structs;
			std::map<String, const Interface*> interfaces;
			std::map<String, const Enum*> enums;
#ifdef LUACAPNP_PARSER
			capnp::SchemaParser parser;
			std::map<const Struct*, capnp::StructSchema> proto;
#endif
		public:
			static Registry& instance();
		
			const Struct* load(StringPtr name, const capnp::StructSchema& s);
			const Interface* load(StringPtr name, const capnp::InterfaceSchema& s);
			const Enum* load(StringPtr name, const capnp::EnumSchema& s);
			const Struct* findStruct(StringPtr name);
			const Interface* findInterface(StringPtr name);
			const Enum* findEnum(StringPtr name);
#ifdef LUACAPNP_PARSER
			const void * parse(const char* name, const char* Name, const char* filename, const char* import, const char* import1);
			const capnp::StructSchema& findCapnpStructSchema(const Struct* s);
#endif
		};
	}
}
