# Private exploration framing and bounded authored additions are copied from
# one verified ZIP into ignored host trees. The MM ROM remains the owner
# of stock textures and untouched geometry. A pending source bundle blocks only
# archive generation, so ordinary executable builds remain available.
get_filename_component(COMBO_NATIVE_FOREST_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(NOT DEFINED COMBO_NATIVE_FOREST_MANIFEST)
    set(COMBO_NATIVE_FOREST_MANIFEST
        "${COMBO_NATIVE_FOREST_SOURCE_ROOT}/tools/native_forest/manifest.json"
        CACHE FILEPATH "Verified native forest source inventory")
endif()
set(_native_forest_extra_args)
if(COMBO_NATIVE_FOREST_BUNDLE)
    list(APPEND _native_forest_extra_args --bundle "${COMBO_NATIVE_FOREST_BUNDLE}")
endif()
add_custom_target(PrepareNativeForestAssets
    COMMAND "${Python3_EXECUTABLE}" -B
        "${COMBO_NATIVE_FOREST_SOURCE_ROOT}/scripts/assets/prepare_native_forest.py"
        --manifest "${COMBO_NATIVE_FOREST_MANIFEST}"
        --output-root "${CMAKE_SOURCE_DIR}"
        ${_native_forest_extra_args}
    COMMENT "Preparing verified native forest framing and authored additions..."
    VERBATIM)
foreach(_native_forest_archive_target GenerateSohOtr Generate2ShipOtr)
    if(TARGET ${_native_forest_archive_target})
        add_dependencies(${_native_forest_archive_target} PrepareNativeForestAssets)
    endif()
endforeach()
