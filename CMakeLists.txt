
project("luacapnp")
cmake_minimum_required(VERSION 3.1)
set(VERSION 0.1-dev)

###############################################################################
# environments

message("CMAKE_GENERATOR=${CMAKE_GENERATOR}")
message("CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
message("CMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}")

set(CMAKE_PREFIX_PATH ${GAG_LIB_PATH})
message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

set(CMAKE_PROGRAM_PATH ${GAG_BIN_PATH})
message("CMAKE_PROGRAM_PATH=${CMAKE_PROGRAM_PATH}")

set(CMAKE_MODULE_PATH "${GAG_CMAKE_MODULE_PATH}")
message("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")


#set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")

include(GNUInstallDirs)

###############################################################################
# find libraries

find_library(TOLUA_LIB tolua)

find_library(BEHAVIAC_LIB behaviac)
set(BEHAVIAC_LIBRARIES
  ${BEHAVIAC_LIB}
)

find_package(CapnProto REQUIRED)
include_directories(${CAPNP_INCLUDE_DIRS})
add_definitions(${CAPNP_DEFINITIONS})
#message("CAPNP_LIB_CAPNP=${CAPNP_LIB_CAPNP}")
#always use host bin
set(CAPNP_EXECUTABLE "${GAG_BIN_PATH}/capnp")
set(CAPNPC_CXX_EXECUTABLE "${GAG_BIN_PATH}/capnpc-c++")



###############################################################################

# add source files

set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE header_list ${src_dir}/*.h)
foreach(header_file ${header_list})
	string(REGEX MATCH "^${src_dir}/(.+)/.+$" group ${header_file})
	set(src_path ${CMAKE_MATCH_1})
	if(group)
		string(REPLACE "/" "\\\\" group ${src_path})
		source_group(${group} FILES ${header_file})
	endif()
endforeach()

file(GLOB_RECURSE source_list ${src_dir}/*.c++)

message("source_list=${source_list}")
foreach(source_file ${source_list})
message("source_file=${source_file}")
	string(REGEX MATCH "^${src_dir}/(.+)/.+$" group ${source_file})
	set(src_path ${CMAKE_MATCH_1})
message("group=${group}")
message("src_path=${src_path}")
	if(group)
		string(REPLACE "/" "\\\\" group ${src_path})
		source_group(${group} FILES ${source_file})
	endif()
endforeach()

###############################################################################
# compile options

if(MSVC)
  	add_compile_options(/wo4503 /W3 /WX)
	string(REPLACE "/O2 /Ob1" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
else()
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-strict-aliasing -Wno-sign-compare -Wno-unused-parameter -Wno-extern-c-compat -pthread -std=c++1z -fexceptions -D__STDC_FORMAT_MACROS")
	SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -D_DEBUG")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "$ENV{CXXFLAGS} -O3 -Wall")
endif()

###############################################################################
# add targets

add_library(luacapnp STATIC ${header_list} ${source_list})
install(TARGETS luacapnp DESTINATION lib)
install(FILES luacapnp.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/luacapnp")

