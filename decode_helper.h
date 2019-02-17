#pragma once
#include "schema.h"
#include "ctype.h"

namespace luacapnp
{
	//loadField
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Ref>
	C loadField(capnp::_::StructReader& p, int offset) {
		return p.getDataField<C>(offset * capnp::ELEMENTS);
	}

	template <>
	bool loadField<capnp::schema::Type::VOID>(
			capnp::_::StructReader& p, int offset) {
		return true;
	}
	
	template <>
	kj::ArrayPtr<const char> loadField<capnp::schema::Type::TEXT>(
			capnp::_::StructReader& p, int offset) {
		capnp::_::PointerReader pr = p.getPointerField(offset);
		if (pr.isNull()) {
			return {};
		} else {
			return  pr.getBlob<capnp::Text>(nullptr, 0);
		}
	}
	
	template <>
	kj::ArrayPtr<const kj::byte> loadField<capnp::schema::Type::DATA>(
			capnp::_::StructReader& p, int offset) {
		capnp::_::PointerReader pr = p.getPointerField(offset);
		if (pr.isNull()) {
			return {};
		} else {
			return  pr.getBlob<capnp::Data>(nullptr, 0);
		}
	}
	
	//loadListElement
	template <capnp::schema::Type::Which T, typename C = typename CType<T>::Ref>
	C loadListElement(capnp::_::ListReader& p, int i) {
		return p.getDataElement<C>(i * capnp::ELEMENTS);
	}

	template <>
	bool loadListElement<capnp::schema::Type::VOID>(
			capnp::_::ListReader& p, int i) {
		return true;
	}

	template <>
	kj::ArrayPtr<const char> loadListElement<capnp::schema::Type::TEXT>(
			capnp::_::ListReader& p, int i) {
		capnp::_::PointerReader pr = p.getPointerElement(i);
		if (pr.isNull()) {
			return {};
		} else {
			return pr.getBlob<capnp::Text>(nullptr, 0);
		}
	}

	template <>
	kj::ArrayPtr<const kj::byte> loadListElement<capnp::schema::Type::DATA>(
			capnp::_::ListReader& p, int i) {
		capnp::_::PointerReader pr = p.getPointerElement(i);
		if (pr.isNull()) {
			return {};
		} else {
			return pr.getBlob<capnp::Data>(nullptr, 0);
		}
	}
}

