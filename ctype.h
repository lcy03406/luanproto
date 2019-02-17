#pragma once

namespace luacapnp
{
	template <capnp::schema::Type::Which which>
	struct CType {
	};
	
	#define DECL_CTYPE_X(T, R, C, S) \
	template<> \
	struct CType<capnp::schema::Type::T> { \
		using Ref = R; \
		using Copy = C; \
		static constexpr capnp::ElementSize Size = S; \
		static Ref asRef(const Copy& value) { \
			return value; \
		} \
	};
	
	#define DECL_CTYPE(T, C) DECL_CTYPE_X(T, C, C, capnp::_::elementSizeForType<C>())

	DECL_CTYPE(BOOL, bool)
	DECL_CTYPE(INT8, int8_t)
	DECL_CTYPE(INT16, int16_t)
	DECL_CTYPE(INT32, int32_t)
	DECL_CTYPE(INT64, int64_t)
	DECL_CTYPE(UINT8, uint8_t)
	DECL_CTYPE(UINT16, uint16_t)
	DECL_CTYPE(UINT32, uint32_t)
	DECL_CTYPE(UINT64, uint64_t)
	DECL_CTYPE(FLOAT32, float)
	DECL_CTYPE(FLOAT64, double)
	DECL_CTYPE(ENUM, uint16_t)

	template<>
	struct CType<capnp::schema::Type::VOID> {
		using Ref = bool;
		using Copy = bool;
		static constexpr capnp::ElementSize Size = capnp::ElementSize::VOID;
		static Ref asRef(const Copy& value) {
			return value;
		}
	};
	
	template<>
	struct CType<capnp::schema::Type::TEXT> {
		using Ref = kj::ArrayPtr<const char>;
		using Copy = std::string;
		static constexpr capnp::ElementSize Size = capnp::ElementSize::POINTER;
		static Ref asRef(const Copy& value) {
			return Ref(value.c_str(), value.size());
		}
	};
	
	template<>
	struct CType<capnp::schema::Type::DATA> {
		using Ref = kj::ArrayPtr<const kj::byte>;
		using Copy = std::basic_string<kj::byte>;
		static constexpr capnp::ElementSize Size = capnp::ElementSize::POINTER;
		static Ref asRef(const Copy& value) {
			return Ref(value.c_str(), value.size());
		}
	};

	#define FOR_ALL_TYPES(X) \
		X(capnp::schema::Type::VOID) \
		X(capnp::schema::Type::BOOL) \
		X(capnp::schema::Type::INT8) \
		X(capnp::schema::Type::INT16) \
		X(capnp::schema::Type::INT32) \
		X(capnp::schema::Type::INT64) \
		X(capnp::schema::Type::UINT8) \
		X(capnp::schema::Type::UINT16) \
		X(capnp::schema::Type::UINT32) \
		X(capnp::schema::Type::UINT64) \
		X(capnp::schema::Type::FLOAT32) \
		X(capnp::schema::Type::FLOAT64) \
		X(capnp::schema::Type::ENUM) \
		X(capnp::schema::Type::TEXT) \
		X(capnp::schema::Type::DATA)
}

