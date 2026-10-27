// Exact native middle-tree draw, identified in Prelude by both shared textures
// and the same 32 selected faces in the user's 2026-10-11 screenshots.
// Source: scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_003928
// SHA-256: 5b8a024ccb6cbecb2c87d90cada11c36620d299d391ef9eda1a43eaaaeaa455c
// Primary: misc/scene_texture_08/scene_texture_08_Tex_005800 (RGBA16, 32x32).
// Secondary: misc/scene_texture_08/scene_texture_08_Tex_005000 (IA4, 64x64).
static const std::vector<Gfx> kActualMiddleForestCommands = {
    Command(0x33000000, 0xBEEFBEEF), Command(0x0D377447, 0x9980A175), Command(0x32008010, 0x00000280),
    Command(0x67CC8083, 0x3CC6B73D), Command(0x03000000, 0x0000000E), Command(0xD7000002, 0xFFFFFFFF),
    Command(0xE7000000, 0x00000000), Command(0xFC20AC04, 0xFF0F93FF), Command(0xFA0000FF, 0xFFFFFFFF),
    Command(0xE200001C, 0xC8113078), Command(0xE3001001, 0x00000000), Command(0x20100000, 0x00000000),
    Command(0xA57FB12D, 0x4E5FE119), Command(0xF5100000, 0x07054051), Command(0xE6000000, 0x00000000),
    Command(0xF3000000, 0x073FF100), Command(0xE7000000, 0x00000000), Command(0xF5101000, 0x00054051),
    Command(0xF2000000, 0x0007C07C), Command(0x20700000, 0x00000000), Command(0xF8F8EC3F, 0x310D11B2),
    Command(0xF5700100, 0x0705BC60), Command(0xE6000000, 0x00000000), Command(0xF3000000, 0x073FF200),
    Command(0xE7000000, 0x00000000), Command(0xF5600900, 0x01F5BC60), Command(0xF2000000, 0x010FC0FC),
    Command(0xD9000000, 0x00230405), Command(0x32020040, 0x00000000), Command(0x67CC8083, 0x3CC6B73D),
    Command(0x06000204, 0x00000406), Command(0x06080A0C, 0x00080C0E), Command(0x060E1012, 0x000E1208),
    Command(0x06141618, 0x0014181A), Command(0x061C1E20, 0x001C2022), Command(0x06061614, 0x00061400),
    Command(0x06242628, 0x0024282A), Command(0x062A282C, 0x002A2C2E), Command(0x062E2C30, 0x002E3032),
    Command(0x06340402, 0x00340236), Command(0x06363230, 0x00363034), Command(0x06383A3C, 0x00383C3E),
    Command(0x063A201E, 0x003A1E3C), Command(0x32008010, 0x00000200), Command(0x67CC8083, 0x3CC6B73D),
    Command(0x06000204, 0x00000406), Command(0x0604080A, 0x00040A06), Command(0x06080C0E, 0x00080E0A),
    Command(0xDF000000, 0x00000000),
};

// Exact paired backwall-grass draw selected in the same Prelude screenshots.
// Source: scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_027238
// SHA-256: 8919a40dc64b615c66859e1c403c363766787e927355bada547d24b8cbc07e06
// Primary: scene_texture_08_Tex_007800 (RGBA16, 32x32).
// Secondary: scene_texture_08_Tex_006800 (IA4, 64x64).
static const std::vector<Gfx> kActualBackwallGrassCommands = {
    Command(0x33000000, 0xBEEFBEEF), Command(0x24F2DB56, 0xDAC87F67), Command(0x32008010, 0x000002E0),
    Command(0xA8E951D9, 0x5A09046C), Command(0x03000000, 0x0000000E), Command(0xD7000002, 0xFFFFFFFF),
    Command(0xE7000000, 0x00000000), Command(0xFC20AC04, 0xFF0F93FF), Command(0xFA0000FF, 0xFFFFFFFF),
    Command(0xE200001C, 0xC8104A50), Command(0xE3001001, 0x00000000), Command(0x20100000, 0x00000000),
    Command(0x2CB7C934, 0xFE7EF95C), Command(0xF5100000, 0x07054450), Command(0xE6000000, 0x00000000),
    Command(0xF3000000, 0x073FF100), Command(0xE7000000, 0x00000000), Command(0xF5101000, 0x00054450),
    Command(0xF2000000, 0x0007C07C), Command(0x20700000, 0x00000000), Command(0xC92B85CD, 0xF29B6E37),
    Command(0xF5700100, 0x0705806F), Command(0xE6000000, 0x00000000), Command(0xF3000000, 0x073FF200),
    Command(0xE7000000, 0x00000000), Command(0xF5600900, 0x01F5806F), Command(0xF2000000, 0x010FC0FC),
    Command(0xD9000000, 0x00230405), Command(0x32020040, 0x00000000), Command(0xA8E951D9, 0x5A09046C),
    Command(0x06000204, 0x00000406), Command(0x06080A0C, 0x00080C0E), Command(0x06101214, 0x00101416),
    Command(0x06181A1C, 0x00181C1E), Command(0x06202224, 0x00202426), Command(0x06282A2C, 0x00282C2E),
    Command(0x06303234, 0x00303436), Command(0x06383A3C, 0x00383C3E), Command(0x3200E01C, 0x00000200),
    Command(0xA8E951D9, 0x5A09046C), Command(0x06000204, 0x00000406), Command(0x06060408, 0x0006080A),
    Command(0x060C0A08, 0x000C080E), Command(0x06101214, 0x00101416), Command(0x06181412, 0x0018121A),
    Command(0xDF000000, 0x00000000),
};
