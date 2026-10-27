# Run the production probe in its own DLL against the real engine DLL. The
# lightweight Linux gate cannot prove Windows' separately linked spdlog registries.
add_library(frame_timing_output_probe SHARED EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_LIST_DIR}/../soh/Enhancements/debugger/FrameTimingProbe.cpp)
target_include_directories(frame_timing_output_probe PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(frame_timing_output_probe PRIVATE libultraship)
target_compile_definitions(frame_timing_output_probe PRIVATE __DLL__ NOMINMAX)

add_executable(frame_timing_output_check EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_LIST_DIR}/frame_timing_output_test.cpp)
target_include_directories(frame_timing_output_check PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(frame_timing_output_check PRIVATE frame_timing_output_probe libultraship)
target_compile_definitions(frame_timing_output_check PRIVATE __DLL__ NOMINMAX SDL_MAIN_HANDLED)

foreach(target frame_timing_output_probe frame_timing_output_check)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/frame-timing-check/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/frame-timing-check/$<CONFIG>"
        WINDOWS_EXPORT_ALL_SYMBOLS FALSE)
    set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY
        "$<TARGET_GENEX_EVAL:libultraship,$<TARGET_PROPERTY:libultraship,MSVC_RUNTIME_LIBRARY>>")
endforeach()

if(MSVC)
    target_sources(frame_timing_output_probe PRIVATE ${CMAKE_CURRENT_LIST_DIR}/frame_timing_output_probe.def)
    # The production Release build defines NDEBUG; assertions are the check's gates.
    target_compile_options(frame_timing_output_check PRIVATE /UNDEBUG)
else()
    target_compile_options(frame_timing_output_check PRIVATE -UNDEBUG)
endif()
