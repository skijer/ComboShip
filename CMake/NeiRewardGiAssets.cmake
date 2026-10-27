# The authored RAW|IMG payloads are stored compressed to keep source transport
# small. Restore their exact bytes before either normal port archive is built.
add_custom_target(PrepareNeiRewardGiAssets
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_LIST_DIR}/../tools/reward_gi/unpack_assets.py"
        --assets-root "${CMAKE_CURRENT_SOURCE_DIR}/soh/assets/custom"
        --other-assets-root "${CMAKE_CURRENT_SOURCE_DIR}/mm/assets/custom"
    COMMENT "Restoring and verifying authored 4K reward materials..."
    VERBATIM)
add_dependencies(CheckAssetCollisions PrepareNeiRewardGiAssets)
