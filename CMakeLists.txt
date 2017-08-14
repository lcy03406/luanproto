project("luanproto" CXX)
cmake_minimum_required(VERSION 3.1)
set(VERSION 0.1-dev)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../build")
set(CMAKE_PROGRAM_PATH "${LIBS_PATH}/bin")
set(CMAKE_LIBRARY_PATH "${LIBS_PATH}/lib")
set(CMAKE_INCLUDE_PATH "${LIBS_PATH}/include")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${LIBS_PATH}" CACHE PATH "..." FORCE)
endif()

include(CheckIncludeFileCXX)
include(GNUInstallDirs)
if(MSVC)
  check_include_file_cxx(initializer_list HAS_CXX11)
else()
  check_include_file_cxx(initializer_list HAS_CXX11 "-std=gnu++0x")
endif()
if(NOT HAS_CXX11)
  message(SEND_ERROR "Requires a C++11 compiler and standard library.")
endif()

# these arguments are passed to all install(TARGETS) calls
set(INSTALL_TARGETS_DEFAULT_ARGS
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

find_package(CapnProto REQUIRED)
include_directories(${CAPNP_INCLUDE_DIRS})
add_definitions(${CAPNP_DEFINITIONS})
#find_package(FlatBuffers REQUIRED)



# Options ======================================================================

if(MSVC)
  add_compile_options(/wo4503)
else()
  add_compile_options(-Wall -Wextra -Wno-strict-aliasing -Wno-sign-compare -Wno-unused-parameter)

  if(DEFINED CMAKE_CXX_EXTENSIONS AND NOT CMAKE_CXX_EXTENSIONS)
    message(SEND_ERROR "Cap'n Proto requires compiler-specific extensions (e.g., -std=gnu++11). Please leave CMAKE_CXX_EXTENSIONS undefined or ON.")
  endif()

  if (NOT ANDROID)
    add_compile_options(-pthread)
  endif()
endif()

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
#foreach(CompilerFlag ${CompilerFlags})
#  string(REPLACE "/MT" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
#  string(REPLACE "/MTd" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
#  string(REPLACE "/MDd" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
#endforeach()

# Source =======================================================================

function(serch_target_files name)
	set(target ${name})
	set(target_srcdir "${CMAKE_CURRENT_SOURCE_DIR}/src/${name}")
	set(target_schemas)
	set(target_sources)
	set(target_headers)
	file(GLOB subfiles "${target_srcdir}/*")
	list(SORT subfiles)
	list(APPEND subfiles "${target_srcdir}")
	foreach(subdir ${subfiles})
		if(IS_DIRECTORY  ${subdir} AND NOT DEFINED ${subdir})
			file(GLOB subdir_sources "${subdir}/*.c??")
			file(GLOB subdir_headers "${subdir}/*.h")
			set(${subdir}_sources ${subdir_sources})
			set(${subdir}_headers ${subdir_headers})
			list(APPEND target_sources ${subdir_sources})
			list(APPEND target_headers ${subdir_headers})
			file(GLOB capnp_schemas "${subdir}/*.capnp")
			get_filename_component(group ${subdir} NAME)
			if(capnp_schemas)
				set(${subdir}gen "gen")
				set(CAPNPC_IMPORT_DIRS ${subdir} "${LIBS_PATH}/include")
				set(CAPNPC_OUTPUT_DIR ${subdir}gen)
				set(CAPNPC_SRC_PREFIX ${subdir})
				capnp_generate_cpp(capnpgen_sources capnpgen_headers ${capnp_schemas})
				source_group(${group}gen FILES ${capnpgen_sources} ${capnpgen_headers})
				list(APPEND target_schemas ${capnp_schemas})
				list(APPEND target_sources ${capnpgen_sources})
				list(APPEND target_headers ${capnpgen_headers})
			endif()
			file(GLOB fbs_schemas "${subdir}/*.fbs")
			if(fbs_schemas)
				set(${subdir}gen "gen")
				set(FBS_IMPORT_DIRS ${subdir})
				set(FBS_OUTPUT_DIR ${subdir}gen)
				set(FBS_SRC_PREFIX ${subdir})
				flatbuffers_generate_c_headers(fbsgen_headers ${fbs_schemas})
				source_group(${group}gen FILES ${fbsgen_headers})
				list(APPEND target_schemas ${fbs_schemas})
				list(APPEND target_headers ${fbsgen_headers})
			endif()
			source_group(${group} FILES ${subdir_sources} ${subdir_headers} ${capnp_schemas} ${fbs_schemas})
		endif()
	endforeach()
	if(MSVC)
		set(target_files ${target_sources} ${target_headers} ${target_schemas} PARENT_SCOPE)
	else()
		set(target_files ${target_sources} PARENT_SCOPE)
	endif()
	set(target ${target} PARENT_SCOPE)
	set(${target}_srcdir ${target_srcdir} PARENT_SCOPE)
	set(${target}_files ${target_files} PARENT_SCOPE)
endfunction()

# Target =======================================================================

serch_target_files(libluanproto)
add_library(${target} SHARED ${target_files})
set_target_properties(${target} PROPERTIES COMPILE_DEFINITIONS "LIBBATTLE_EXPORTS")
target_link_libraries(${target} ${CAPNP_LIBRARIES})
install(TARGETS ${target} ${INSTALL_TARGETS_DEFAULT_ARGS})

serch_target_files("tester")
add_executable(${target} WIN32 ${target_files})
add_dependencies(${target} libluanproto)
target_link_libraries(${target} libluanproto)
target_include_directories(${target} PRIVATE ${libBattle_srcdir})
install(TARGETS ${target} ${INSTALL_TARGETS_DEFAULT_ARGS})

