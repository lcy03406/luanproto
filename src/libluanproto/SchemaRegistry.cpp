#include "PCH.h"
#include "SchemaRegistry.h"

bool SchemaRegistry::load()
{
	capnp::SchemaParser parser;
	auto schema = parser.parseDiskFile()
}

const capnp::ParsedSchema* SchemaRegistry::find()
{

}
