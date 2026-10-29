cmake_minimum_required(VERSION 3.24)
project(Fleet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# The combined randomizer engine: game-agnostic headers that talk to each game through JSON.
add_library(fleet-rando INTERFACE)
target_include_directories(fleet-rando INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/combo)
target_link_libraries(fleet-rando INTERFACE nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(fleet-rando INTERFACE /bigobj /utf-8)
endif()

enable_testing()
add_executable(combo_shared_test combo/tests/combo_shared_test.cpp)
target_link_libraries(combo_shared_test PRIVATE fleet-rando)
add_test(NAME combo_shared_test COMMAND combo_shared_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
