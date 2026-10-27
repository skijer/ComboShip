# The full chamber dependency closure is committed as verified compressed parts.
# Preparing the normal ports does not require the external Prelude scene addon.
get_filename_component(COMBO_TEMPLE_SAGES_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_custom_target(PrepareTempleSagesAssets
    COMMAND "${Python3_EXECUTABLE}" -B
        "${COMBO_TEMPLE_SAGES_SOURCE_ROOT}/scripts/assets/prepare_temple_sages.py"
        --output-root "${CMAKE_SOURCE_DIR}"
    COMMENT "Preparing built-in Sages Chamber geometry, collision and materials..."
    VERBATIM)
foreach(_temple_sages_archive_target GenerateSohOtr Generate2ShipOtr)
    if(TARGET ${_temple_sages_archive_target})
        add_dependencies(${_temple_sages_archive_target} PrepareTempleSagesAssets)
    endif()
endforeach()
