# Retained geometry and original surfaces stay verified in git. Both ordinary
# port-archive packers consume them; no supplemental market archive is needed.
get_filename_component(COMBO_NATIVE_MARKET_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB COMBO_NATIVE_MARKET_TEXTURE_PARTS CONFIGURE_DEPENDS
    "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/props.zlib.*.b64"
    "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/textures.zlib.*.b64"
    "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/shelf.zlib.*.b64")
add_custom_target(
    PrepareNativeMarketAssets
    COMMAND ${Python3_EXECUTABLE} -B "${COMBO_NATIVE_MARKET_ROOT}/scripts/assets/prepare_native_market.py"
    WORKING_DIRECTORY "${COMBO_NATIVE_MARKET_ROOT}"
    DEPENDS "${COMBO_NATIVE_MARKET_ROOT}/scripts/assets/prepare_native_market.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/native_market.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/market_interior.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/room_magic.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/room_magic/provenance.json"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/prop_appearance.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/market_title.py"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/exit_door.py"
            "${COMBO_NATIVE_MARKET_ROOT}/combo/BlackMarketExitDoor.h"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/title/black-market-wordmark-original.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/title/black-market-wordmark.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/title/black-market-title.ia8"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/title/provenance.json"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/timber.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/slate.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/plaster.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/ceramic.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/bronze.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/wicker.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/celestial.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/herbs.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/curios.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/liquid.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/cloth.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/props-provenance.json"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/props-encoded.json"
            ${COMBO_NATIVE_MARKET_TEXTURE_PARTS}
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/encoded.json"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/arcane_stone.png"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/shelf-encoded.json"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/prop-glass.json"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/black_market/assets/interior/prop-glass.zlib.001.b64"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/native_market/bundled/resources.zlib.b64"
            "${COMBO_NATIVE_MARKET_ROOT}/tools/native_market/bundled/manifest.json"
    COMMENT "Preparing native Black Market geometry and original interior surfaces..."
    VERBATIM)

