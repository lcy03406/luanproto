#pragma once
//#ifdef _WINDOWS
//#include <SDKDDKVer.h>
//#define WIN32_LEAN_AND_MEAN
//#define NOMINMAX
//#include <windows.h>
//#pragma warning(disable: 4996)
//#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <bitset>
#include <functional>
#include <iostream>

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
#include "Lib.h"
