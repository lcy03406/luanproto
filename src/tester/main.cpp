#include <fstream>
#include <stdio.h>
#include "capnpgen/ATypes.capnp.h"

template <typename T>
class CapnpIsStruct
{
private:
	template <typename X, typename = decltype(T{ ::capnp::_::StructReader{} })>
	static constexpr bool isReader(X*) { return true; }
	template <typename X, typename = decltype(T{ ::capnp::_::StructBuilder{} })>
	static constexpr bool isBuilder(X*) { return true; }
	template <typename X>
	static constexpr bool isReader(...) { return false; }
	template <typename X>
	static constexpr bool isBuilder(...) { return false; }
public:
	static const bool value = isReader<T>(nullptr) || isBuilder<T>(nullptr);
};

//template <typename T, typename = typename std::enable_if<CapnpIsStruct<std::remove_reference_t<T>>::value>::type>
template <typename T>
inline bool CapnpIsNull(const T& reader)
{
	static_assert(CapnpIsStruct<std::remove_reference_t<T>>::value, "type should be a capnproto struct");
	//if T is a struct reader or builder, its first member is a segment pointer
	//TODO avoid the hack
	return *reinterpret_cast<const void*const*>(&reader) == nullptr;
}

int main(int argc, char** argv)
{
	BlackWood::Generated::AnyValue::Builder builder(nullptr);
	printf("Reader %d %d\n", CapnpIsStruct<BlackWood::Generated::AnyValue::Reader>::value, CapnpIsNull(builder));
	printf("Builder %d\n", CapnpIsStruct<BlackWood::Generated::AnyValue::Builder>::value);
	printf("AnyValue %d\n", CapnpIsStruct<BlackWood::Generated::AnyValue>::value);
	printf("AsPlanId %d\n", CapnpIsStruct<BlackWood::Generated::AnyValue::AsPlanId::Reader>::value);
	printf("AsVec2 %d\n", CapnpIsStruct<BlackWood::Generated::AnyValue::AsVec2::Reader>::value);
	printf("AsTuple %d\n", CapnpIsStruct<BlackWood::Generated::AnyValue::AsTuple::Reader>::value);
	printf("List %d\n", CapnpIsStruct<decltype(BlackWood::Generated::AnyValue::AsTuple::Reader{}.getData()) > ::value);
	return 0;
}

