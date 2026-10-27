#!/usr/bin/env python3
"""Verify native season confirmation, cancellation and the bounded switch flash."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_mm_weather_tests import production_function

ROOT = Path(__file__).resolve().parents[2]


def main():
    rod = (ROOT / "mm/mods/items/logic/item_rod_of_seasons.c").read_text()
    fixture = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "z64.h"
#include "mods/extended_inventory.h"
#include "mods/items/helpers/box_menu.c"
PlayState* gPlayState;
static u8 chosen = SEASON_SUMMER;
static unsigned flashes, refreshes;
static u8 lastAlpha;
u8 Seasons_GetSeason(void) { return chosen; }
void Seasons_SetSeason(u8 value) {
    if (value == SEASON_AUTUMN || value == SEASON_SUMMER || value == SEASON_OFF) chosen = value;
}
void ExtInv_RefreshButtonIconsForItem(PlayState* play, u16 item) {
    assert(play == gPlayState && item == EXT_ITEM_ROD_OF_SEASONS);
    ++refreshes;
}
void Audio_PlaySfx(u16 sfx) {}
void Environment_DrawLightningFlash(PlayState* play, u8 r, u8 g, u8 b, u8 alpha) {
    assert(play == gPlayState && r == 255 && g == 245 && b == 225 && alpha <= 128);
    ++flashes; lastAlpha = alpha;
}
'''
    fixture += production_function(rod, "Seasons_OnWheelConfirm")
    fixture += r'''
static void Open(PlayState* play, s32 target) {
    BoxMenuEntry entries[5];
    for (unsigned i = 0; i < 5; ++i) entries[i] = (BoxMenuEntry){ "test/icon", 32, 1 };
    play->state.input[0].press.button = 0;
    assert(BoxMenu_Open(play, entries, 5, target, 0, Seasons_OnWheelConfirm));
}
int main(void) {
    PlayState play = {0}, other = {0};
    play.sceneId = SCENE_00KEIKOKU;
    play.gameplayFrames = 100;
    gPlayState = &play;
    Open(&play, SEASON_AUTUMN);
    play.state.input[0].press.button = BTN_B;
    BoxMenu_Update(&play);
    assert(!BoxMenu_IsOpen() && chosen == SEASON_SUMMER && sBoxMFlashPlay == NULL && refreshes == 0);
    Open(&play, SEASON_AUTUMN);
    play.state.input[0].press.button = BTN_A;
    BoxMenu_Update(&play);
    assert(!BoxMenu_IsOpen() && chosen == SEASON_AUTUMN && refreshes == 1 && sBoxMFlashPlay == &play);
    const u8 expected[] = {48, 96, 128, 104, 72, 44, 20, 0};
    for (unsigned i = 0; i < sizeof(expected); ++i) {
        play.gameplayFrames = 100 + i;
        BoxMenu_DrawSeasonFlash(&play);
        assert(flashes == i + 1 && lastAlpha == expected[i]);
    }
    ++play.gameplayFrames;
    BoxMenu_DrawSeasonFlash(&play);
    assert(flashes == sizeof(expected) && sBoxMFlashPlay == NULL);
    Open(&play, SEASON_AUTUMN);
    play.state.input[0].press.button = BTN_A;
    BoxMenu_Update(&play);
    assert(sBoxMFlashPlay == NULL); // Reconfirming the same season has no flash.
    Open(&play, SEASON_WINTER);
    play.state.input[0].press.button = BTN_A;
    BoxMenu_Update(&play);
    assert(chosen == SEASON_AUTUMN && sBoxMFlashPlay == NULL); // Rejected selection.
    Open(&play, SEASON_OFF);
    play.state.input[0].press.button = BTN_A;
    BoxMenu_Update(&play);
    assert(chosen == SEASON_OFF && sBoxMFlashPlay == &play);
    BoxMenu_DrawSeasonFlash(&play);
    assert(lastAlpha == 48);
    for (unsigned invalid = 0; invalid < 7; ++invalid) {
        play.gameplayFrames = 200;
        play.sceneId = SCENE_00KEIKOKU;
        play.roomCtx.curRoom.num = 0;
        play.pauseCtx.state = PAUSE_STATE_OFF;
        play.csCtx.state = CS_STATE_IDLE;
        play.transitionTrigger = TRANS_TRIGGER_OFF;
        play.transitionMode = TRANS_MODE_OFF;
        MM_BoxMenu_StartSeasonFlash(&play);
        unsigned before = flashes;
        if (invalid == 0) ++play.sceneId;
        if (invalid == 1) ++play.roomCtx.curRoom.num;
        if (invalid == 2) --play.gameplayFrames;
        if (invalid == 3) play.pauseCtx.state = PAUSE_STATE_MAIN;
        if (invalid == 4) play.csCtx.state = 1;
        if (invalid == 5) play.transitionTrigger = TRANS_TRIGGER_START;
        if (invalid == 6) play.transitionMode = TRANS_MODE_INSTANCE_RUNNING;
        BoxMenu_DrawSeasonFlash(&play);
        assert(flashes == before && sBoxMFlashPlay == NULL);
    }
    MM_BoxMenu_StartSeasonFlash(&play);
    unsigned before = flashes;
    BoxMenu_DrawSeasonFlash(&other);
    assert(flashes == before && sBoxMFlashPlay == NULL);
    puts("PASS native A/B season confirmation, reset, same/rejected selection and bounded scene/room-owned flash");
}
'''
    with tempfile.TemporaryDirectory(prefix="mm-season-selector-") as temporary:
        build = Path(temporary)
        source = build / "selector.c"
        source.write_text(fixture)
        binary = build / "test"
        flags = ["-std=gnu17", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0", "-ffunction-sections", "-fdata-sections"]
        flags += ["-I" + str(ROOT / path) for path in
                  ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "libultraship/include", "combo")]
        extra = shlex.split(os.environ.get("MM_FOLIAGE_TEST_CXXFLAGS", ""))
        result = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags, *extra,
                                 str(source), "-Wl,--gc-sections", "-o", str(binary)], text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
