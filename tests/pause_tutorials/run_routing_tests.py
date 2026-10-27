#!/usr/bin/env python3
"""Execute MM's actual equipment cursor and observe its textbox boundary.

The inherited ownership fixture disables optional sword enhancements through a
typed boundary; the live chooser has separate real-header production tests.
"""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/mm_equipment_pause"))
import run_ownership_tests as ownership


def fixture():
    source = ownership.fixture()
    source = source[:source.index("int main() {")]
    source = source.replace("int BrokenItems_FormCount() { return 0; }", "int BrokenItems_FormCount() { return 3; }")
    source = source.replace("u8 BrokenItems_FormUnlocked(int) { return 0; }", "u8 BrokenItems_FormUnlocked(int) { return 1; }")
    source = source.replace("void BrokenItems_EquipForm(PlayState*, int) {}", "void BrokenItems_EquipForm(PlayState*, int) { ++equipWrites; }")
    source = source.replace("u8 PauseItemDesc_ShowEquipment(PlayState*, s16, s16, s16, u8) { return 0; }",
                            "static int descCalls, lastPage, lastRow, lastCol, lastForm=-1;\n"
                            "u8 PauseItemDesc_ShowEquipment(PlayState*, s16 page, s16 row, s16 col, u8) {"
                            "++descCalls;lastPage=page;lastRow=row;lastCol=col;return 1;}")
    source = source.replace("u8 PauseItemDesc_ShowForm(PlayState*, s32, u8) { return 0; }",
                            "u8 PauseItemDesc_ShowForm(PlayState*, s32 form, u8) {++descCalls;lastForm=form;return 1;}")
    return source + r'''
int main() {
    Reset(); PlayState play; sEquipSubPage=1;
    for(int row=0;row<4;++row) for(int col=1;col<=3;++col) ExtEquip_GiveItem(row,col);
    for(int row=0;row<4;++row) for(int col=1;col<=3;++col) {
        sEquipCursorY=row;sEquipCursorX=col;play.pauseCtx.itemDescriptionOn=0;
        play.state.input.press.button=BTN_CUP;
        int calls=descCalls, writes=equipWrites;
        KaleidoScope_UpdateEquipmentCursor(&play);
        check(descCalls==calls+1&&lastPage==1&&lastRow==row&&lastCol==col,"MM actual C-Up action routes every extended cell");
        check(play.pauseCtx.itemDescriptionOn&&equipWrites==writes,"C-Up opens description without equipping or granting");
    }
    for(int form=0;form<3;++form) {
        sEquipSubPage=2;sTransformCursor=form;play.pauseCtx.itemDescriptionOn=0;
        int calls=descCalls,writes=equipWrites;play.state.input.press.button=BTN_CUP;
        KaleidoScope_UpdateEquipmentCursor(&play);
        check(descCalls==calls+1&&lastForm==form,"MM actual form-page C-Up opens selected controls");
        check(play.pauseCtx.itemDescriptionOn&&equipWrites==writes,"form C-Up leaves equipped form unchanged");
    }
    // The kaleido dispatcher still calls this cursor while a textbox is open.
    // Every action must yield to that textbox until it closes.
    sEquipSubPage=2;sTransformCursor=1;play.pauseCtx.itemDescriptionOn=1;
    const NeiSaveData before=save;int writes=equipWrites,calls=descCalls;
    play.state.input.press.button=BTN_A;
    KaleidoScope_UpdateEquipmentCursor(&play);
    check(equipWrites==writes,"A cannot equip a form during its description");
    play.state.input.press.button=BTN_L;
    KaleidoScope_UpdateEquipmentCursor(&play);
    check(sEquipSubPage==2,"L cannot switch equipment pages during a description");
    play.state.input.press.button=BTN_CUP;
    KaleidoScope_UpdateEquipmentCursor(&play);
    check(descCalls==calls,"C-Up does not reopen an active description");
    check(std::memcmp(&before,&save,sizeof(save))==0,"description inputs preserve all acquisition/save state");
    play.pauseCtx.itemDescriptionOn=0;play.state.input.press.button=BTN_A;
    KaleidoScope_UpdateEquipmentCursor(&play);
    check(equipWrites==writes+1,"A still equips forms after the description closes");
    std::printf("MM pause tutorial input routing: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
'''


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--sanitize",action="store_true")
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="pause-routing-") as folder:
        cpp=Path(folder)/"routing.cpp";exe=Path(folder)/"routing"
        cpp.write_text(fixture())
        command=[os.environ.get("CXX","c++"),"-std=c++20","-I"+str(ROOT),"-I"+str(ROOT/"mm"),"-I"+str(ROOT/"combo"),
                 "-I"+str(ROOT/"combo/menu"),"-I"+str(ROOT/"mm/assets"),str(cpp),"-o",str(exe)]
        if args.sanitize:
            command += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-g"]
        subprocess.run(command,check=True)
        subprocess.run([str(exe)],check=True)


if __name__=="__main__":
    main()
