cmake_minimum_required(VERSION 3.20)
project(tinycpng C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Disable zlib-ng's own tests and examples
set(WITH_GTEST OFF CACHE BOOL "Disable building tests for zlib-ng")
set(WITH_FUZZERS OFF CACHE BOOL "Disable building examples for zlib-ng")
set(WITH_BENCHMARKS OFF CACHE BOOL "Disable building benchmarks for zlib-ng")
set(WITH_BENCHMARK_APPS OFF CACHE BOOL "Disable building benchmarks-apps for zlib-ng")
set(ZLIB_COMPAT OFF CACHE BOOL "Disable zlib-ng zlib compatibility layer")
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ZLIBNG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ZLIBNG_ENABLE_PACKAGE_CONFIG OFF CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_OPTIMIZATIONS ON CACHE BOOL "Enable zlib-ng optimizations")

FetchContent_Declare(
        zlib-ng
        GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
        GIT_TAG 2.3.2
        EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(zlib-ng)

set(CMAKE_C_STANDARD 11)

add_library(tinycpng SHARED
        src/library/library.c
        src/library/png_crc.c
        src/library/zlibng_wrapper.c
        src/library/png_helpers.c
        src/library/png_filters.c
        src/library/png_chunks.c
        src/library/png_palette.c
        src/library/png_read.c
        src/library/png_write.c
)

add_library(tinycpng-static STATIC
        src/library/library.c
        src/library/png_crc.c
        src/library/zlibng_wrapper.c
        src/library/png_helpers.c
        src/library/png_filters.c
        src/library/png_chunks.c
        src/library/png_palette.c
        src/library/png_read.c
        src/library/png_write.c
)

add_library(tinycpng_utils STATIC
        src/library/file_utils.c
        src/library/helpers.c
        src/library/error_utils.c
)

target_compile_definitions(tinycpng PRIVATE TINYCPNG_EXPORTS=1)
#target_compile_options(tinycpng PUBLIC -fms-extensions)
target_include_directories(tinycpng PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/public>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(tinycpng PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/private)

target_include_directories(tinycpng-static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/public>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(tinycpng-static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/private)

target_include_directories(tinycpng_utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/public>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(tinycpng_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/private)
target_link_libraries(tinycpng PRIVATE zlib-ng-static)
target_link_libraries(tinycpng PUBLIC  tinycpng_utils)

target_link_libraries(tinycpng-static PRIVATE zlib-ng-static)
target_link_libraries(tinycpng-static PUBLIC  tinycpng_utils)

add_executable(reader src/png_reader.c)

target_link_libraries(reader PUBLIC tinycpng)

add_executable(tests
        src/tests/png_tests.c
        src/tests/filter_tests.c

        src/library/png_filters.c
        src/library/png_helpers.c
        src/library/png_chunks.c
        src/library/png_crc.c
        src/tests/encoding_decoding_tests.c
)
target_link_libraries(tests PUBLIC tinycpng tinycpng_utils)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycpng/private)
