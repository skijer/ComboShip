#!/usr/bin/env python3
"""Guard native hook ordering and MM's real normal-message/credits boundary."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]

class Integration(unittest.TestCase):
    def test_arrival_precedes_native_player_and_camera(self):
        for host in ("soh", "mm"):
            with self.subTest(host=host):
                source = (ROOT / host / "src/code/z_play.c").read_text()
                start = source.index("void Play_Init(")
                prepare = source.index("MagicMirror_PrepareArrival(", start)
                spawn = source.index("Actor_InitContext(", start)
                finish = source.index("MagicMirror_FinishArrival(", start)
                camera = source.index("Camera_Init", spawn)
                self.assertLess(prepare, spawn)
                self.assertLess(spawn, finish)
                self.assertLess(finish, camera)

    def test_prompt_uses_normal_mm_decoder(self):
        source = (ROOT / "combo/gameplay/MagicMirrorRuntime.inc.h").read_text()
        prompt = int(re.search(r"kPromptText = (0x[\dA-Fa-f]+)", source)[1], 16)
        native = (ROOT / "mm/src/code/z_message.c").read_text()
        credits = int(re.search(r"textId >= (0x[\dA-Fa-f]+)", native)[1], 16)
        self.assertLess(prompt, credits, "MM treats this prompt as credits even with a custom message loader")

    def test_press_is_claimed_before_native_item_change(self):
        for host in ("soh", "mm"):
            with self.subTest(host=host):
                path = "soh/Enhancements/Gameplay" if host == "soh" else "2s2h/Enhancements/Gameplay"
                source = (ROOT / host / path / "MagicMirror.cpp").read_text()
                self.assertTrue("RegisterGameHook<GameInteractor::OnGameStateMainStart>(MirrorRuntime::Update)" in source,
                                host + " Mirror input is processed after native item changes")
                native = (ROOT / host / "src/code/game.c").read_text()
                start = native.index("void GameState_Update(")
                self.assertLess(native.index("GameInteractor_ExecuteOnGameStateMainStart()", start),
                                native.index("gameState->main(gameState)", start))

if __name__ == "__main__":
    unittest.main()
