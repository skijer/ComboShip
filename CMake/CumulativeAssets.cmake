# Preserve exact runtime and authoring inputs through bounded text source parts.
get_filename_component(COMBO_CUMULATIVE_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_custom_target(PrepareCumulativeAssets
    COMMAND "${Python3_EXECUTABLE}" -B
        "${COMBO_CUMULATIVE_SOURCE_ROOT}/scripts/assets/prepare_cumulative_assets.py"
        --output-root "${CMAKE_SOURCE_DIR}"
    COMMENT "Preparing exact cumulative assets from verified compressed source parts..."
    VERBATIM)
foreach(_cumulative_asset_target GenerateSohOtr Generate2ShipOtr CheckAssetCollisions)
    if(TARGET ${_cumulative_asset_target})
        add_dependencies(${_cumulative_asset_target} PrepareCumulativeAssets)
    endif()
endforeach()
