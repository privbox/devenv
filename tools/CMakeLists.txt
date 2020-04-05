cmake_minimum_required(VERSION 3.16)

project(PIOT)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(_DEBUG)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

include(GNUInstallDirs)

find_library(STATIC_LIBC
    NAMES
        libc.a
    DOC
        "static libc"
    REQUIRED
)
if(${STATIC_LIBC} STREQUAL "STATIC_LIBC-NOTFOUND")
    message(FATAL_ERROR "static libc is required, but not found!")
endif(${STATIC_LIBC} STREQUAL "STATIC_LIBC-NOTFOUND")

add_subdirectory(microtester)
