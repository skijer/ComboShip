# This material is used by the source-owned half-floor partition, independently
# of external Lake scene archives. Keep scene/grass atlases out of this bundle.
get_filename_component(COMBO_LAKE_SAND_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_custom_target(PrepareLakeTunnelSandAssets
    COMMAND "${Python3_EXECUTABLE}" -B
        "${COMBO_LAKE_SAND_ROOT}/scripts/assets/prepare_lake_tunnel_sand.py"
        --output-root "${CMAKE_SOURCE_DIR}"
    DEPENDS "${COMBO_LAKE_SAND_ROOT}/scripts/assets/prepare_lake_tunnel_sand.py"
        "${COMBO_LAKE_SAND_ROOT}/tools/lake_tunnel_sand/manifest.json"
        "${COMBO_LAKE_SAND_ROOT}/tools/lake_tunnel_sand/bundled/manifest.json"
        "${COMBO_LAKE_SAND_ROOT}/tools/cumulative_assets/cumulative_source_pool.py"
    COMMENT "Preparing dedicated Lake Hylia tunnel sand in native and Alt paths..."
    VERBATIM)
if(TARGET GenerateSohOtr)
    add_dependencies(GenerateSohOtr PrepareLakeTunnelSandAssets)
endif()
