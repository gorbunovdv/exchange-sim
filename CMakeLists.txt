cmake_minimum_required(VERSION 4.2.3)

project(exchange-sim)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("CMAKE_INTERPROCEDURAL_OPTIMIZATION: ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")

add_compile_options(
    "$<$<CONFIG:Debug>:-O0;-g>"
    "$<$<CONFIG:Release>:-O3;-march=native>"
    "$<$<CONFIG:RelWithDebInfo>:-O3;-g;-march=native;-fno-omit-frame-pointer>"
)

# todo(gorbunovdv): Add additional options
add_compile_options(
    -Wall -Wextra -Werror -Wpedantic
)

set(SANITIZER "none" CACHE STRING "none|address|thread|undefined")

message("SANITIZER: ${SANITIZER}")

if(NOT SANITIZER STREQUAL "none")
    add_compile_options(-fno-omit-frame-pointer -g -fno-sanitize-recover=all)
endif()
if(SANITIZER STREQUAL "address")
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
elseif(SANITIZER STREQUAL "thread")
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
elseif(SANITIZER STREQUAL "undefined")
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
elseif(NOT SANITIZER STREQUAL "none")
    message(FATAL_ERROR "Unknown sanitizer type: ${SANITIZER}")
endif ()

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(base)
add_subdirectory(domain)
add_subdirectory(exchange)
if(EXISTS "${PROJECT_SOURCE_DIR}/extra")
    add_subdirectory(extra)
endif()
