# Keep the large RGBA resource compressed in source; both native packers consume
# the identical decoded resource through their ordinary custom-asset trees.
get_filename_component(COMBO_CHATEAU_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB COMBO_CHATEAU_PACKED_PARTS CONFIGURE_DEPENDS
     "${COMBO_CHATEAU_SOURCE_ROOT}/tools/chateau_romani_3ds/bundled/AtlasTex.zlib.*.b64")
add_custom_target(
    PrepareChateauRomaniAssets
    COMMAND ${Python3_EXECUTABLE} "${COMBO_CHATEAU_SOURCE_ROOT}/scripts/assets/prepare_chateau_romani.py"
    WORKING_DIRECTORY "${COMBO_CHATEAU_SOURCE_ROOT}"
    DEPENDS "${COMBO_CHATEAU_SOURCE_ROOT}/scripts/assets/prepare_chateau_romani.py"
            ${COMBO_CHATEAU_PACKED_PARTS}
            "${COMBO_CHATEAU_SOURCE_ROOT}/tools/chateau_romani_3ds/bundled/manifest.json"
            "${COMBO_CHATEAU_SOURCE_ROOT}/tools/chateau_romani_3ds/manifest.json"
    COMMENT "Preparing bundled Chateau Romani HD assets..."
    VERBATIM)
