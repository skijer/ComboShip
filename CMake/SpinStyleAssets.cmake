# The source pool preserves exact resource bytes while bounding each upload.
get_filename_component(COMBO_SPIN_STYLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(NOT DEFINED COMBO_SPIN_STYLE_OUTPUT_ROOT)
    set(COMBO_SPIN_STYLE_OUTPUT_ROOT "${COMBO_SPIN_STYLE_ROOT}")
endif()
file(GLOB COMBO_SPIN_STYLE_PARTS "${COMBO_SPIN_STYLE_ROOT}/tools/spin_styles/bundled/*.b64")
add_custom_target(
    PrepareSpinStyleAssets
    COMMAND ${Python3_EXECUTABLE} -B "${COMBO_SPIN_STYLE_ROOT}/scripts/assets/prepare_spin_styles.py"
            --output-root "${COMBO_SPIN_STYLE_OUTPUT_ROOT}"
    WORKING_DIRECTORY "${COMBO_SPIN_STYLE_ROOT}"
    DEPENDS "${COMBO_SPIN_STYLE_ROOT}/scripts/assets/prepare_spin_styles.py"
            "${COMBO_SPIN_STYLE_ROOT}/tools/spin_styles/bundled/manifest.json"
            ${COMBO_SPIN_STYLE_PARTS}
    COMMENT "Preparing exact private spin-style resources..."
    VERBATIM)
