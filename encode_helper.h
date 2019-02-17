#pragma once
#include "schema.h"
#include "ctype.h"

namespace luacapnp
{
	//saveField
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Ref>
	void saveField(capnp::_::StructBuilder& p, int offset, C value) {
		p.setDataField<C>(offset * capnp::ELEMENTS, value);
	}
	
	template <>
	void saveField<capnp::schema::Type::VOID>(capnp::_::StructBuilder& p, int offset, bool value) {
	}
	
	template <>
	void saveField<capnp::schema::Type::TEXT>(capnp::_::StructBuilder& p, int offset, kj::ArrayPtr<const char> value) {
		capnp::_::PointerBuilder pb = p.getPointerField(offset);
		size_t size = value.size();
		auto data = value.begin();
		capnp::Text::Builder lb = pb.initBlob<capnp::Text>(size);
		memcpy(lb.begin(), data, size);
	}
	
	template <>
	void saveField<capnp::schema::Type::DATA>(capnp::_::StructBuilder& p, int offset, kj::ArrayPtr<const kj::byte> value) {
		capnp::_::PointerBuilder pb = p.getPointerField(offset);
		size_t size = value.size();
		auto data = value.begin();
		capnp::Data::Builder lb = pb.initBlob<capnp::Data>(size);
		memcpy(lb.begin(), data, size);
	}

	//saveListElement
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Ref>
	void saveListElement(capnp::_::ListBuilder& lb, int i, const C value) {
		lb.setDataElement<C>(i * capnp::ELEMENTS, value);
	}

	template <>
	void saveListElement<capnp::schema::Type::VOID>(capnp::_::ListBuilder& lb, int i, bool value) {
	}

	template <>
	void saveListElement<capnp::schema::Type::TEXT>(capnp::_::ListBuilder& lb, int i, const kj::ArrayPtr<const char> value) {
		capnp::_::PointerBuilder pb = lb.getPointerElement(i * capnp::ELEMENTS);
		size_t size = value.size();
		auto data = value.begin();
		capnp::Text::Builder bb = pb.initBlob<capnp::Text>(size);
		memcpy(bb.begin(), data, size);
	}

	template <>
	void saveListElement<capnp::schema::Type::DATA>(capnp::_::ListBuilder& lb, int i, const kj::ArrayPtr<const kj::byte> value) {
		capnp::_::PointerBuilder pb = lb.getPointerElement(i * capnp::ELEMENTS);
		size_t size = value.size();
		auto data = value.begin();
		capnp::Data::Builder bb = pb.initBlob<capnp::Data>(size);
		memcpy(bb.begin(), data, size);
	}
}

