#!/usr/bin/env python3
"""Check keeper copy through OoT's actual pure AutoFormatString and width table.

This isolates the formatter from unavailable logging/item-metadata dependencies.
It is not the full native OoT decoder/metadata gate in run_text_codec_tests.py.
"""
from pathlib import Path
import os
import subprocess
import tempfile
from run_text_codec_tests import function, common_flags

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'soh/soh/Enhancements/custom-message/CustomMessageManager.cpp').read_text()
# Exact production tables and functions. Only debug logging is an inert boundary.
tables = source[source.index('using namespace std::literals'):source.index('CustomMessage::CustomMessage(')]
signatures = (
    'CustomMessage::CustomMessage(std::string english_, TextBoxType',
    'static size_t NextLineLength(', 'size_t CustomMessage::FindNEWLINE(',
    'bool CustomMessage::AddBreakString(', 'void CustomMessage::AutoFormatString(',
    'void CustomMessage::ReplaceSpecialCharacters(', 'void CustomMessage::EncodeColors(',
    'void CustomMessage::ReplaceColors(', 'void CustomMessage::ReplaceAltarIcons(',
    'std::string CustomMessage::MESSAGE_END(', 'std::string CustomMessage::ITEM_OBTAINED(',
    'std::string CustomMessage::NEWLINE(', 'std::string CustomMessage::COLOR(',
    'std::string CustomMessage::WAIT_FOR_INPUT(', 'std::string CustomMessage::PLAYER_NAME(',
)
body = r'''
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "combo/BlackMarketText.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <map>
#define SPDLOG_DEBUG(...) ((void)0)
'''
body += tables + '\n' + '\n'.join(function(source, signature) for signature in signatures)
body += r'''
int main() {
    using BlackMarketText::Page;
    CustomMessage formatter("");
    for (int language = 0; language < 3; ++language) {
      for (Page page : { Page::Welcome, Page::KeeperTalk, Page::KeeperHelpChoice, Page::KeeperHelp }) {
        std::string text = BlackMarketText::Oot(page, {}, {}, language);
        formatter.AutoFormatString(text);
        text = BlackMarketText::OotPages(text);
        unsigned line = 1, pages = 1, choiceLine = 0, choices = 0, events = 0, aGlyphs = 0;
        int pixels = 0;
        for (size_t i = 0; i < text.size();) {
          const uint8_t c = text[i];
          if (c == 0x01 || c == 0x04) {
            assert(pixels <= 216); pixels = 0;
            if (c == 0x04) { line = 1; ++pages; } else ++line;
          } else if (c == 0x1B) { choiceLine = line; ++choices; }
          else if (c == 0x0B) ++events;
          else if (c == 0x9F) { ++aGlyphs; pixels += 12; }
          else if (c == 0xA0) pixels += 12;
          else if (c >= 0x20) {
            assert(pixelWidthTable.contains(text.substr(i, 1)));
            pixels += pixelWidthTable.at(text.substr(i, 1));
          }
          assert(line <= 4);
          const size_t command = ComboReceipt_CommandSize(c, false);
          assert(command <= text.size() - i);
          i += command;
        }
        assert(pixels <= 216);
        assert(choices == (page == Page::KeeperHelpChoice ? 1 : 0));
        assert(events == (page == Page::KeeperHelpChoice ? 0 : 1));
        if (choices) assert(choiceLine == 3 && line == 4);
        if (page == Page::KeeperTalk) assert(pages == 1 && text.find("No guessing games") != std::string::npos);
        if (page == Page::KeeperHelp) assert(pages == 3 && text.find("unique dungeon clears") != std::string::npos &&
                                            text.find("empty bottle") != std::string::npos);
        if (page == Page::Welcome) assert(aGlyphs == 1 && text.find("Talk to shopkeeper") != std::string::npos);
      }
    }
    std::puts("PASS OoT actual pure formatter/table: dialogue pages, 216px widths, native choice rows, EVENT/A glyph (ASan/UBSan); full decoder gate remains separate");
}
'''
with tempfile.TemporaryDirectory(prefix='keeper-oot-formatter-') as folder:
    cpp = Path(folder) / 'test.cpp'
    cpp.write_text(body)
    binary = Path(folder) / 'test'
    includes = [ROOT, ROOT / 'combo', ROOT / 'combo/menu', ROOT / 'soh', ROOT / 'soh/include', ROOT / 'soh/include/PR', ROOT / 'soh/soh',
                ROOT / 'libultraship/include']
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *common_flags(includes),
                    str(cpp), '-no-pie', '-Wl,--gc-sections', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
