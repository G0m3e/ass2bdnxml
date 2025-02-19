cmake_minimum_required(VERSION 3.20)
project(ass2bdnxml LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_TOOLCHAIN_FILE "ENV${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MINGW)
        set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
        set(VCPKG_HOST_TRIPLET "x64-mingw-static")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-windows")
        set(VCPKG_HOST_TRIPLET "x64-windows")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(VCPKG_TARGET_TRIPLET "x64-linux")
    set(VCPKG_HOST_TRIPLET "x64-linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(VCPKG_TARGET_TRIPLET "x64-osx")
    set(VCPKG_HOST_TRIPLET "x64-osx")
endif()

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    add_compile_definitions("BE_ARCH")
else()
    add_compile_definitions("LE_ARCH")
endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBASS REQUIRED IMPORTED_TARGET libass)

set(SOURCES
    auto_split.c
    palletize.c
    sort.c
    sup.c
    common.c)

add_executable(ass2bdnxml ass2bdnxml.c ${SOURCES})

target_link_libraries(ass2bdnxml 
    PRIVATE 
    ZLIB::ZLIB 
    PNG::PNG
    vfw32
    PkgConfig::LIBASS)

add_executable(avs2bdnxml avs2bdnxml.c ${SOURCES})

target_link_libraries(avs2bdnxml
    PRIVATE
    ZLIB::ZLIB
    PNG::PNG
    vfw32
    PkgConfig::LIBASS)

add_library(ass2sup ${SOURCES}
    ass2sup.c)

target_include_directories(ass2sup PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:ass2sup/include>
)

target_link_libraries(ass2sup 
    PRIVATE 
    ZLIB::ZLIB 
    PNG::PNG
    vfw32
    PkgConfig::LIBASS)
