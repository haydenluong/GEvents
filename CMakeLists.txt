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

add_executable(gevents_benchmark bench/main.cpp src/benchmark.cpp src/event_loader.cpp
               src/trie.cpp src/hash_table.cpp)
target_include_directories(gevents_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(gevents_benchmark PRIVATE /W4)
else()
    target_compile_options(gevents_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(raymond_tests tests/test_raymond.cpp src/hash_table.cpp src/ranking.cpp)
    target_include_directories(raymond_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    if(MSVC)
        target_compile_options(raymond_tests PRIVATE /W4)
    else()
        target_compile_options(raymond_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME hash_table_and_ranking COMMAND raymond_tests)
endif()
