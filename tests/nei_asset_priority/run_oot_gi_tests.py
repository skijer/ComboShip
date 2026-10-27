"""Production OoT legacy GI fallbacks keep live mod models and materials."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags
source = (ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text()
def function(name):
    match = re.search(r"^static (?:Gfx\*|void) " + name + r"\([^;{}]*\) \{", source, re.M)
    assert match, name
    i, depth = match.end(), 1
    while depth:
        depth += (source[i] == "{") - (source[i] == "}")
        i += 1
    return source[match.start():i]
with tempfile.TemporaryDirectory(prefix="oot-gi-mod-") as directory:
    build = Path(directory)
    (build / "oot_gi_fallback.inc").write_text("\n".join(function(name) for name in (
        "NeiGi_ModOverrideDL", "LoadMmDLOnce", "Pegasus_GetRecoloredBootsDL", "BuildRecoloredGiDL", "DrawCustomItemDiamondByPath")))
    binary = str(build / "test")
    subprocess.run(["c++", *flags(), "-I" + directory, str(ROOT / "tests/nei_asset_priority/oot_gi_fallback_test.cpp"), "-o", binary], check=True)
    subprocess.run([binary], check=True)
print("PASS: OoT GI mod fallback, local/donor ownership, live Alt, missing recovery and custom boot materials")
