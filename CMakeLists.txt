cmake_minimum_required(VERSION 3.16)
project(GEvents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB GEVENTS_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(gevents ${GEVENTS_SOURCES})
target_include_directories(gevents PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(gevents PRIVATE /W4)
else()
    target_compile_options(gevents PRIVATE -Wall -Wextra -Wpedantic)
endif()
