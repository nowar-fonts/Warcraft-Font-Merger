cmake_minimum_required(VERSION 3.12)
project(WFM_all)
include(CheckLibraryExists)

set(WFM_all_VERSION_MAJOR 1)
set(WFM_all_VERSION_MINOR 1)
set(WFM_all_VERSION_PATCH 0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

configure_file(
	config/config.h.in
	config/config.h
)

configure_file(
	config/config.sh.in
	config/config.sh
)

include_directories(
	"${PROJECT_BINARY_DIR}/config"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/include/dep"
	"${PROJECT_SOURCE_DIR}/src/otfcc"
)

file(GLOB_RECURSE src_otfcc src/otfcc/**.c)
file(GLOB_RECURSE src_otfcc_extern src/otfcc-extern/**.c)


add_executable(otfccdump
	${src_otfcc}
	${src_otfcc_extern}
	src/otfcc-driver/otfccdump.c
	src/otfcc-driver/stopwatch.c
)

add_executable(otfccbuild
	${src_otfcc}
	${src_otfcc_extern}
	src/otfcc-driver/otfccbuild.c
	src/otfcc-driver/stopwatch.c
)

add_executable(merge-otd
	src/merger/merge-name.cpp
	src/merger/merge-otd.cpp
	src/merger/ps2tt.cpp
	src/merger/tt2ps.cpp
	src/nowide/iostream.cpp
)

check_library_exists(m sin "" require_lib_m)
if (require_lib_m)
	target_link_libraries(otfccdump m)
	target_link_libraries(otfccbuild m)
	target_link_libraries(merge-otd m)
endif (require_lib_m)
