#include "emotes/EmotePlayer.h"
#include "emotes/EmoteAudio.h"
#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <stdexcept>

using namespace ComboEmotes;
static void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
int main(int argc, char** argv) {
    Check(kCatalog.size() == 8, "all eight supplied animations are selectable");
    Check(kModifier1 == 0x40, "default shortcuts use the existing M1 virtual button");
    std::array<uint8_t, kFrameBytes * 2> bytes{};
    bytes[0] = 0xff; bytes[1] = 0xfe; bytes[2] = 0x12; bytes[3] = 0x34;
    bytes[kFrameBytes] = 0x80; bytes[kFrameBytes + 1] = 0;
    Pose pose{};
    Check(DecodeFrame(bytes, 0, pose) && pose.joints[0] == -2 && pose.joints[1] == 0x1234,
          "big-endian signed translations and rotations decode");
    Check(DecodeFrame(bytes, 1, pose) && pose.joints[0] == -32768, "second frame uses 134-byte stride");
    Check(!DecodeFrame(bytes, 2, pose) && !DecodeFrame(std::span(bytes).first(133), 0, pose),
          "out-of-bounds and truncated frames are rejected");
    std::array<uint8_t, 159> raw;
    raw.fill(0x5a);
    pose.joints[0] = -123; pose.appearance = 0x123;
    Check(WritePose(raw, 9, pose), "MM aligned joint table fits its 159-byte allocation");
    int16_t root = 0;
    std::memcpy(&root, raw.data() + 9, 2);
    Check(root == -123 && raw[8] == 0x5a && raw[143] == 0x5a,
          "wire buffer writes preserve alignment padding and native byte order");
    Check(!WritePose(raw, 26, pose), "offset cannot overflow a raw network buffer");
    auto malformed = bytes;
    malformed[132] = 3;
    Check(!DecodeFrame(malformed, 0, pose), "unsupported MM hand appearance is rejected before native drawing");
    Check(!ValidateClip(malformed, 2), "an invalid frame rejects the complete clip");
    Check(ValidateClip(bytes, 2), "a valid complete clip passes validation");
    for (double dt : { 1.0 / 20, 1.0 / 30, 1.0 / 60 }) {
        Playback player;
        Check(player.Start(1, 20), "known emote starts");
        for (int i = 0; i < std::lround(0.5 / dt); ++i) player.Tick(dt);
        Check(player.Frame() == 10, "playback is 20 Hz independently of simulation rate");
        for (int i = 0; i < std::lround(0.5 / dt); ++i) player.Tick(dt);
        Check(!player.Active(), "one-shot playback ends without reading an extra frame");
        Check(!player.Start(99, 20) && !player.Start(1, 0), "invalid IDs and empty streams fail closed");
        player.Start(1, 20); player.Tick(0.4); player.Start(2, 8);
        Check(player.Id() == 2 && player.Frame() == 0, "replacement starts at frame zero");
        player.Stop(); Check(!player.Active(), "cancel leaves no active frame");
    }
    std::array<uint32_t, 4> chords{ 0x40 | 0x800, 0x40 | 0x400, 0, 0 };
    Check(SelectBinding(chords, chords[0], 0x800) == 0, "modifier followed by direction triggers");
    Check(SelectBinding(chords, chords[0], 0) == -1, "held chord never retriggers");
    Check(SelectBinding(chords, 0x800, 0x800) == -1, "partial chord leaves normal controls alone");
    Check(SelectBinding(chords, 0, 0) == -1, "unbound slots do not fire");
    chords[0] = 0x1000;
    Check(SelectBinding(chords, 0x1000, 0x1000) == -1, "Start remains available for the pause menu");
    chords[0] = 0x80000000;
    Check(SelectBinding(chords, 0x80000000, 0x80000000) == -1, "invalid persisted chord bits fail closed");
    std::array<int16_t, 3> pcm{ 32767, -32768, 20 };
    AudioVoice voice;
    voice.Start(pcm);
    std::array<int16_t, 8> mix{ 200, 200, -200, -200, 0, 0, 7, 7 };
    voice.Mix(mix.data(), 4, 1.0f);
    Check(mix[0] == 32767 && mix[2] == -32768 && mix[4] == 20 && mix[6] == 7,
          "mono sound mixes to stereo, saturates and ends at its actual length");
    voice.Start(pcm); voice.Stop(); mix.fill(5); voice.Mix(mix.data(), 4, 1.0f);
    Check(mix[0] == 5, "stop prevents stale sound after cancellation");
    for (int i = 1; i < argc; ++i) {
        std::ifstream file(argv[i], std::ios::binary);
        std::vector<uint8_t> stream{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        Check(!stream.empty() && stream.size() % kFrameBytes == 0, "bundled stream is complete");
        for (size_t frame = 0; frame < stream.size() / kFrameBytes; ++frame)
            Check(DecodeFrame(stream, frame, pose), "every supplied frame decodes");
        Check(!DecodeFrame(stream, stream.size() / kFrameBytes, pose), "bundled stream stops at its last frame");
    }
    std::cout << "PASS emote decoding, clocks, replacement/cancel, chords and audio\n";
}
