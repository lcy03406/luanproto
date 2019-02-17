#include "pch.h"
#include "schema.h"
namespace luacapnp {
	namespace schema {
		void Type::load(RegistryData* regdata, const capnp::Type& t) {
			auto s = t;
			type = s.which();
			for (; type == CapnpType::LIST; depth++) {
				s = s.asList().getElementType();
				type = s.which();
			}
			switch (type) {
				case CapnpType::ENUM:
					enumType = regdata->load(s.asEnum());
					break;
				case CapnpType::STRUCT:
					structType = regdata->load(s.asStruct());
					break;
				default:
					break;
			}
		}

		void Enum::load(RegistryData* regdata, const capnp::EnumSchema& s) {
			auto es = s.getEnumerants();
			enumerants.reserve(es.size());
			for (auto e : es) {
				auto ordinal = e.getOrdinal();
				auto name = e.getProto().getName();
				enumerants.push_back(name);
				index[name] = ordinal;
			}
		}
		
		unsigned Enum::tointeger(StringPtr name) const {
			auto it = index.find(name);
			if (it == index.end()) {
				return 0;
			}
			return it->second;
		}


		void Struct::load(RegistryData* regdata, const capnp::StructSchema& s) {
			auto p = s.getProto().getStruct();
			size.data = p.getDataWordCount() * capnp::WORDS;
			size.pointers = p.getPointerCount() * capnp::POINTERS;
			hasUnion = p.getDiscriminantCount() > 0;
			discriminantOffset = p.getDiscriminantOffset();
			isGroup = p.getIsGroup();
			auto fl = s.getFields();
			for (auto fs : fl) {
				auto t = fs.getType();
				auto fp = fs.getProto();
				if (fp.isGroup()) {
					auto& f = groups[fp.getName()];
					f.discriminantValue = fp.getDiscriminantValue();
					f.inUnion = f.discriminantValue != capnp::schema::Field::NO_DISCRIMINANT;
					f.type.load(regdata, t.asStruct()); 
				} else {
					auto& f = slots[fp.getName()];
					f.discriminantValue = fp.getDiscriminantValue();
					f.inUnion = f.discriminantValue != capnp::schema::Field::NO_DISCRIMINANT;
					f.offset = fp.getSlot().getOffset();
					f.type.load(regdata, t);
					f.load(regdata, fp);
				}
			}
		}

		void Interface::load(RegistryData* regdata, const capnp::InterfaceSchema& s) {
			auto ms = s.getMethods();
			methods.reserve(ms.size());
			for (auto m : ms) {
				auto& method = methods.emplace_back(Method());
				method.name = m.getProto().getName();
				method.ordinal = m.getOrdinal();
				method.param = regdata->load(m.getParamType());
				method.result = regdata->load(m.getResultType());
				index[method.name] = &method;
			}
		}
		
		const Method* Interface::getMethod(unsigned ordinal) const {
			if (ordinal >= methods.size())
				return nullptr;
			return &methods[ordinal];
		}
		
		const Method* Interface::getMethod(StringPtr name) const {
			auto it = index.find(name);
			if (it == index.end())
				return nullptr;
			return it->second;
		}

		
		void SlotField::load(RegistryData* regdata, const capnp::schema::Field::Reader& fp) {
			if (fp.hasAnnotations()) {
				for (auto anno : fp.getAnnotations())
				{
					auto id = anno.getId();
					if (id == 0xe3caf55077c0bd92)
					{
						mapkey = anno.getValue().getText();
						ismap = true;
					}
					else if (id == 0x8e28078f594c7ec5)
					{
						mapvalue = anno.getValue().getText();
					}
				}
			}
		}

		const Enum* RegistryData::load(const capnp::EnumSchema& s) {
			auto& m = enums;
			auto id = s.getProto().getId();
			auto [it, ins] = m.emplace(id, Enum());
			if (ins) {
				it->second.load(this, s);
			}
			return &it->second;
		}

		const Struct* RegistryData::load(const capnp::StructSchema& s) {
			auto& m = structs;
			auto id = s.getProto().getId();
			auto [it, ins] = m.emplace(id, Struct());
			if (ins) {
				it->second.load(this, s);
			}
			auto p = &it->second;
			return p;
		}

		const Interface* RegistryData::load(const capnp::InterfaceSchema& s) {
			auto& m = interfaces;
			auto id = s.getProto().getId();
			auto [it, ins] = m.emplace(id, Interface());
			if (ins) {
				it->second.load(this, s);
			}
			auto p = &it->second;
			return p;
		}

		const Struct* Registry::load(StringPtr name, const capnp::StructSchema& s) {
			auto* loaded = data.load(s);
			if (loaded == nullptr)
				return nullptr;
			structs[name] = loaded;
#ifdef LUACAPNP_PARSER
			proto[loaded] = s;
#endif
			return loaded;
		}

		const Interface* Registry::load(StringPtr name, const capnp::InterfaceSchema& s) {
			auto* loaded = data.load(s);
			if (loaded == nullptr)
				return nullptr;
			interfaces[name] = loaded;
#ifdef LUACAPNP_PARSER
			auto ms = s.getMethods();
			for (size_t i = 0; i < ms.size(); ++i) {
				auto m = ms[i];
				auto& method = loaded->methods[i];
				proto[method.param] = m.getParamType();
				proto[method.result] = m.getResultType();
			}
#endif
			return loaded;
		}
		
		const Enum* Registry::load(StringPtr name, const capnp::EnumSchema& s) {
			auto* loaded = data.load(s);
			if (loaded == nullptr)
				return nullptr;
			enums[name] = loaded;
			return loaded;
		}
		
		const Struct* Registry::findStruct(StringPtr name) {
			auto it = structs.find(name);
			if (it == structs.end())
				return nullptr;
			return it->second;
		}
		
		const Interface* Registry::findInterface(StringPtr name) {
			auto it = interfaces.find(name);
			if (it == interfaces.end())
				return nullptr;
			return it->second;
		}
		
		const Enum* Registry::findEnum(StringPtr name) {
			auto it = enums.find(name);
			if (it == enums.end())
				return nullptr;
			return it->second;
		}
		
#ifdef LUACAPNP_PARSER
		const void * Registry::parse(const char* name, const char* Name, const char* filename, const char* import, const char* import1) {
			kj::StringPtr importPathName[2] = {import, import1};
			auto importPath = kj::arrayPtr(importPathName, 2);
			auto fileSchema = parser.parseDiskFile(filename, filename, importPath);
			auto schema = KJ_REQUIRE_NONNULL(fileSchema.findNested(Name));
			auto proto = schema.getProto();
			if (proto.isInterface()) {
				return load(name, schema.asInterface());
			} else if (proto.isStruct()) {
				return load(Name, schema.asStruct());
			} else if (proto.isEnum()) {
				return load(Name, schema.asEnum());
			} else {
				return nullptr;
			}
		}
		
		const capnp::StructSchema& Registry::findCapnpStructSchema(const Struct* s) {
			auto it = proto.find(s);
			return it->second;
		}
#endif

		Registry& Registry::instance() {
			static Registry inst;
			return inst;
		}
	}
}
