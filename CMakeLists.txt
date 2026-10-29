cmake_minimum_required(VERSION 3.24)

option(FLEET_BUILD_HOST "Build the Fleet programs, which need libultraship" ON)

project(Fleet LANGUAGES C CXX)

# Fleet binds both games to its own libultraship, so it must be built exactly like theirs.
if(FLEET_BUILD_HOST AND WIN32)
    include(libultraship/cmake/automate-vcpkg.cmake)
    set(VCPKG_TRIPLET x64-windows-static-md)
    set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
    vcpkg_bootstrap()
    vcpkg_install_packages(zlib bzip2 libzip libpng sdl2 sdl2-net glew glfw3 nlohmann-json tinyxml2 spdlog)
endif()

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

if(FLEET_BUILD_HOST)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(LUS_BUILD_SHARED ON)
    set(GBI_UCODE F3DEX_GBI_2)
    set(GBI_FLOAT_MTX ON)
    set(GBI_S32_VTX ON)
    set(INCLUDE_MPQ_SUPPORT ON)
    set(GFX_DEBUG_DISASSEMBLER ON)
    add_compile_definitions(CONTROLLERBUTTONS_T=uint32_t)
    include(cmake/lus-cvars.cmake)
    add_subdirectory(libultraship ${CMAKE_BINARY_DIR}/libultraship SYSTEM)
    target_compile_definitions(libultraship PUBLIC INCLUDE_MPQ_SUPPORT)

    add_executable(fleet-seedgen src/GameModule.cpp src/seedgen/main.cpp)
    target_include_directories(fleet-seedgen PRIVATE src)
    target_link_libraries(fleet-seedgen PRIVATE libultraship fleet-rando)
    target_compile_definitions(fleet-seedgen PRIVATE NOMINMAX)
    if(MSVC)
        set_target_properties(fleet-seedgen PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
    add_custom_command(TARGET fleet-seedgen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libultraship> $<TARGET_FILE_DIR:fleet-seedgen>)
endif()
