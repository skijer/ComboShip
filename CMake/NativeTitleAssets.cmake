# Native typography is part of the ordinary build. HD art is only an override.
get_filename_component(COMBO_NATIVE_TITLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_custom_target(
    PrepareNativeTitleAssets
    COMMAND ${Python3_EXECUTABLE} -B "${COMBO_NATIVE_TITLE_ROOT}/scripts/assets/prepare_native_titles.py"
    WORKING_DIRECTORY "${COMBO_NATIVE_TITLE_ROOT}"
    DEPENDS "${COMBO_NATIVE_TITLE_ROOT}/scripts/assets/prepare_native_titles.py"
            "${COMBO_NATIVE_TITLE_ROOT}/tools/location_titles/native_assets.py"
            "${COMBO_NATIVE_TITLE_ROOT}/tools/location_titles/assets/native-v3/manifest.json"
            "${COMBO_NATIVE_TITLE_ROOT}/tools/location_titles/assets/native-v3/cards.zlib.b64"
    COMMENT "Preparing built-in native OoT location-card typography..."
    VERBATIM)
