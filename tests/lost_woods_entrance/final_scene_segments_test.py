#!/usr/bin/env python3
"""Bounded regression tests for the FINAL Lost Woods native segment repair."""
import importlib.util
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "tools/lost_woods_entrance/final_scene_segments.py"
tool = None
if MODULE.exists():
    spec = importlib.util.spec_from_file_location("final_scene_segments", MODULE)
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)


def header(magic, version=0):
    return struct.pack("<B3xIIQ", 0, int.from_bytes(magic, "little"), version,
                       0xDEADBEEFDEADBEEF).ljust(64, b"\0")


def command(w0, w1=0):
    return struct.pack("<II", w0, w1)


def display_list(*commands, ucode=4):
    return header(b"TLDO") + bytes([ucode]) + b"\xff" * 7 + b"".join(commands)


def animation(*segments):
    return header(b"NATO") + struct.pack("<I", len(segments)) + b"".join(
        struct.pack("<bhbbBB", segment, 0, -1, 1, 64, 64) for segment in segments
    ) + struct.pack("<I", 0)


class SegmentRepairTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(tool, "the bounded native segment repair is not implemented")

    def repair(self, data):
        return tool.repair_display_list(data, "custom/prelude/Z2_LOST_WOODS/test", {8, 9, 10, 11, 12, 13})

    def test_repairs_only_address_flag_without_reading_hash_payload_as_commands(self):
        # Losing expanded-command framing would mistakenly patch the fake DE
        # in this marker payload; omitting the fix leaves the real low pointer.
        original = display_list(command(0x33000000, 0xBEEFBEEF),
                                command(0xDE000000, 0x0C000000),
                                command(0xDE000000, 0x0B000000), command(0xDF000000))
        fixed, changes = self.repair(original)
        self.assertEqual([i for i, (a, b) in enumerate(zip(original, fixed)) if a != b], [92])
        self.assertEqual(struct.unpack_from("<I", fixed, 92)[0], 0x0B000001)
        self.assertEqual(len(changes), 1)
        self.assertEqual(changes[0]["command_offset"], 88)

    def test_already_native_flagged_call_is_unchanged(self):
        original = display_list(command(0xDE000000, 0x0A000001), command(0xDF000000))
        self.assertEqual(self.repair(original), (original, []))

    def test_push_and_terminal_branch_both_receive_native_segment_flag(self):
        for mode in (0xDE000000, 0xDE010000):
            with self.subTest(mode=hex(mode)):
                fixed, changes = self.repair(display_list(command(mode, 0x09000000), command(0xDF000000)))
                self.assertEqual(struct.unpack_from("<II", fixed, 72), (mode, 0x09000001))
                self.assertEqual(changes[0]["mode"], "push" if mode == 0xDE000000 else "branch")

    def test_nonzero_offset_is_rejected_instead_of_assuming_native_gfx_size(self):
        for address in (0x08000008, 0x08000009):
            with self.subTest(address=hex(address)), self.assertRaisesRegex(ValueError, "offset"):
                self.repair(display_list(command(0xDE000000, address), command(0xDF000000)))

    def test_out_of_range_or_unbound_segment_is_rejected(self):
        for address in (0, 0x07000000, 0x0E000000, 0x10000000, 0xFFFFFFFF):
            with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                self.repair(display_list(command(0xDE000000, address), command(0xDF000000)))

    def test_branch_with_dead_commands_and_invalid_push_mode_are_rejected(self):
        for commands in ((command(0xDE010000, 0x08000000), command(0xE7000000), command(0xDF000000)),
                         (command(0xDE020000, 0x08000000), command(0xDF000000))):
            with self.subTest(commands=commands), self.assertRaises(ValueError):
                self.repair(display_list(*commands))

    def test_hash_display_list_branch_is_rejected_because_native_handler_asserts(self):
        with self.assertRaisesRegex(ValueError, "hash.*branch"):
            self.repair(display_list(command(0x31010000), command(0x12345678, 0xABCDEF01), command(0xDF000000)))

    def test_all_native_expanded_commands_require_their_hash_word(self):
        for opcode in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42):
            with self.subTest(opcode=hex(opcode)), self.assertRaises(ValueError):
                self.repair(display_list(command(opcode << 24)))

    def test_end_opcode_inside_hash_does_not_terminate_stream(self):
        original = display_list(command(0x33000000), command(0xDF000000),
                                command(0xDE000000, 0x08000000), command(0xDF000000))
        fixed, changes = self.repair(original)
        self.assertEqual(len(changes), 1)
        self.assertEqual(struct.unpack_from("<I", fixed, 92)[0], 0x08000001)

    def test_unsupported_microcode_opcode_truncation_and_trailing_data_are_rejected(self):
        samples = [display_list(command(0xDF000000), ucode=ucode) for ucode in (0, 1, 2, 3, 5, 255)]
        samples += [display_list(command(0x7F000000), command(0xDF000000)),
                    display_list(command(0xE7000000)), display_list(command(0xDF000000)) + b"\0" * 8,
                    display_list(command(0xDF000000))[:-1]]
        for sample in samples:
            with self.subTest(sample=sample.hex()), self.assertRaises(ValueError):
                self.repair(sample)

    def test_texanim_proves_native_bindings_and_terminal_negative_segment(self):
        self.assertEqual(tool.animation_segments(animation(1, 2, 3, 4, 5, -6), "anim"), set(range(8, 14)))
        for bad in (animation(1, 2), animation(-1, -2), animation(1, -1), animation(1, -9),
                    animation(0), animation(-1)[:-1]):
            with self.subTest(bad=bad.hex()), self.assertRaises(ValueError):
                tool.animation_segments(bad, "anim")

    def test_archive_candidate_is_deterministic_and_receipt_accounts_for_every_changed_byte(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            source = temp / "source.o2r"
            original = {
                "alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSTexAnim_001BD0": animation(-4),
                "custom/prelude/Z2_LOST_WOODS/test": display_list(command(0xDE000000, 0x0B000000), command(0xDF000000)),
                "prelude/project/edits.json": b'{"edits":{"actors":"preserve"}}',
                "geometry-textures-and-header": bytes(range(256)),
            }
            with zipfile.ZipFile(source, "w") as archive:
                for name, data in original.items():
                    archive.writestr(name, data)
            original_zip = source.read_bytes()
            cli_before = subprocess.run([sys.executable, "-B", str(MODULE), str(source)], capture_output=True, text=True)
            self.assertEqual(cli_before.returncode, 1, cli_before.stdout + cli_before.stderr)
            first, second = temp / "first.o2r", temp / "second.o2r"
            receipt = tool.repair_archive(source, first)
            tool.repair_archive(source, second)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            self.assertEqual(source.read_bytes(), original_zip)
            self.assertEqual(receipt["changed_resources"], 1)
            self.assertEqual(receipt["changed_bytes"], 1)
            self.assertEqual(receipt["changes"][0]["byte_offset"], 76)
            self.assertEqual(receipt["changes"][0]["before_byte"], 0)
            self.assertEqual(receipt["changes"][0]["after_byte"], 1)
            self.assertIn("before_sha256", receipt["changed_resource_hashes"]["custom/prelude/Z2_LOST_WOODS/test"])
            self.assertEqual(receipt["changed_resource_hashes"]["custom/prelude/Z2_LOST_WOODS/test"]["before_sha256"],
                             hashlib.sha256(original["custom/prelude/Z2_LOST_WOODS/test"]).hexdigest())
            self.assertEqual(tool.verify_archive(first)["unflagged_calls"], 0)
            cli_after = subprocess.run([sys.executable, "-B", str(MODULE), str(first)], capture_output=True, text=True)
            self.assertEqual(cli_after.returncode, 0, cli_after.stdout + cli_after.stderr)
            with zipfile.ZipFile(first) as archive:
                self.assertEqual(set(archive.namelist()), set(original))
                for name, data in original.items():
                    if name != "custom/prelude/Z2_LOST_WOODS/test":
                        self.assertEqual(archive.read(name), data)
            with self.assertRaises(ValueError):
                tool.repair_archive(source, source)
            with self.assertRaises(FileExistsError):
                tool.repair_archive(source, first)

    def test_unknown_commands_leave_no_partial_archive(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            source, target = temp / "source.o2r", temp / "target.o2r"
            with zipfile.ZipFile(source, "w") as archive:
                archive.writestr("alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSTexAnim_001BD0", animation(-1))
                archive.writestr("custom/prelude/Z2_LOST_WOODS/test", display_list(command(0x7F000000), command(0xDF000000)))
            with self.assertRaises(ValueError):
                tool.repair_archive(source, target)
            self.assertFalse(target.exists())


class NativeSourceProbe(unittest.TestCase):
    def test_actual_native_segaddr_and_dl_handler_skip_old_addresses_and_dispatch_flagged_addresses(self):
        compiler = shutil.which("g++")
        self.assertIsNotNone(compiler, "g++ is required for the native interpreter regression probe")
        source = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()

        def function(signature):
            start = source.index(signature)
            brace = source.index("{", start)
            depth, end = 1, brace + 1
            while depth:
                depth += (source[end] == "{") - (source[end] == "}")
                end += 1
            return source[start:end]

        probe = r'''
#include <cstdint>
#include <cassert>
#include <unordered_set>
#define COMBO_BUILD 1
#define SPDLOG_ERROR(...) ((void)0)
struct F3DGfx { struct { uintptr_t w0, w1; } words; };
struct Interpreter { uintptr_t mSegmentPointers[16]{}; void* SegAddr(uintptr_t); };
struct Stack { int calls=0, branches=0; F3DGfx* target=nullptr;
  void call(F3DGfx*, F3DGfx* t) { ++calls; target=t; }
  void branch(F3DGfx*) { ++branches; }
} g_exec_stack;
static F3DGfx* ComboResolveDisplayListTarget(uintptr_t, F3DGfx* target) { return target; }
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << (width)) - 1))
'''
        probe += function("void* Interpreter::SegAddr(")
        probe += function("static bool ComboIsUnresolvedSegmentTarget(")
        probe += function("bool gfx_dl_handler_common(")
        probe += r'''
int main() {
  Interpreter gfx; F3DGfx bound[2]{};
  for (uintptr_t segment=8; segment<=13; ++segment) {
    gfx.mSegmentPointers[segment]=reinterpret_cast<uintptr_t>(bound);
    for (uintptr_t mode : {uintptr_t(0xDE000000), uintptr_t(0xDE010000)}) {
      F3DGfx old{{mode, segment<<24}}; F3DGfx* pc=&old; g_exec_stack={};
      assert(gfx.SegAddr(old.words.w1)==reinterpret_cast<void*>(old.words.w1));
      assert(!gfx_dl_handler_common(&gfx, &pc));
      assert(g_exec_stack.calls==0 && g_exec_stack.branches==0);
      F3DGfx repaired{{mode, (segment<<24)|1}}; pc=&repaired; g_exec_stack={};
      assert(gfx.SegAddr(repaired.words.w1)==bound);
      bool branched=gfx_dl_handler_common(&gfx, &pc);
      if (mode==0xDE000000) assert(!branched && g_exec_stack.calls==1 && g_exec_stack.target==bound);
      else assert(branched && g_exec_stack.branches==1 && pc==bound);
    }
    gfx.mSegmentPointers[segment]=0;
    F3DGfx unbound{{0xDE000000, (segment<<24)|1}}; F3DGfx* pc=&unbound; g_exec_stack={};
    assert(!gfx_dl_handler_common(&gfx, &pc) && g_exec_stack.calls==0);
  }
}
'''
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            cpp, exe = temp / "native-segments.cpp", temp / "native-segments"
            cpp.write_text(probe)
            subprocess.run([compiler, "-std=c++20", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
