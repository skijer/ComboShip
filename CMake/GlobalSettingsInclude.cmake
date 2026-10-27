# Probe-only hook for Kenix3/libultraship#1236 (Jameriquiah e3cd6591).
# ComboShip includes this file optionally before add_subdirectory(libultraship),
# so the rebased patch is either verified in the integrated LUS source or applied
# before any LUS targets are configured or compiled.

find_program(_COMBO_SCROLL_GIT git REQUIRED)
set(_COMBO_SCROLL_PATCH
    "${CMAKE_CURRENT_SOURCE_DIR}/patches/libultraship-custom-tex-scroll-e3cd6591.patch")

execute_process(
    COMMAND "${_COMBO_SCROLL_GIT}" apply --check --directory=libultraship "${_COMBO_SCROLL_PATCH}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _COMBO_SCROLL_CHECK_RESULT
    OUTPUT_QUIET
    ERROR_VARIABLE _COMBO_SCROLL_CHECK_ERROR)

if(_COMBO_SCROLL_CHECK_RESULT EQUAL 0)
    execute_process(
        COMMAND "${_COMBO_SCROLL_GIT}" apply --directory=libultraship "${_COMBO_SCROLL_PATCH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _COMBO_SCROLL_APPLY_RESULT
        OUTPUT_QUIET
        ERROR_VARIABLE _COMBO_SCROLL_APPLY_ERROR)

    if(NOT _COMBO_SCROLL_APPLY_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to apply libultraship custom texture scrolling probe (e3cd6591):\n"
            "${_COMBO_SCROLL_APPLY_ERROR}")
    endif()

    message(STATUS "Applied libultraship custom texture scrolling probe e3cd6591")
else()
    # Source integration and repeated configuration must contain the complete
    # patch, not just names that could survive a partial merge.
    execute_process(
        COMMAND "${_COMBO_SCROLL_GIT}" apply --reverse --check --directory=libultraship "${_COMBO_SCROLL_PATCH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _COMBO_SCROLL_REVERSE_RESULT
        OUTPUT_QUIET
        ERROR_VARIABLE _COMBO_SCROLL_REVERSE_ERROR)

    if(_COMBO_SCROLL_REVERSE_RESULT EQUAL 0)
        message(STATUS "libultraship custom texture scrolling probe e3cd6591 already applied")
    else()
        message(FATAL_ERROR
            "libultraship custom texture scrolling probe e3cd6591 neither applies cleanly "
            "nor matches the integrated patch. Refusing to patch an unexpected LUS tree. "
            "git apply --check reported:\n${_COMBO_SCROLL_CHECK_ERROR}"
            "git apply --reverse --check reported:\n${_COMBO_SCROLL_REVERSE_ERROR}")
    endif()
endif()

unset(_COMBO_SCROLL_REVERSE_ERROR)
unset(_COMBO_SCROLL_REVERSE_RESULT)
unset(_COMBO_SCROLL_APPLY_ERROR)
unset(_COMBO_SCROLL_APPLY_RESULT)
unset(_COMBO_SCROLL_CHECK_ERROR)
unset(_COMBO_SCROLL_CHECK_RESULT)
unset(_COMBO_SCROLL_PATCH)
unset(_COMBO_SCROLL_GIT)
