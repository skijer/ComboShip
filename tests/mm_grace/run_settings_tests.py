"""Execute the real combo seed normalize/restore/save-application paths."""
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions, block_from

def source(path):
    if "--baseline" in sys.argv:
        text = subprocess.check_output(["git", "show", "af54f452:" + path], cwd=ROOT, text=True)
    else:
        text = (ROOT / path).read_text(encoding="utf-8-sig")
    return text.replace('extern "C" ', '')

soh = functions(source("soh/soh/OTRGlobals.cpp"), {"SOH_NormalizeComboGraceFromMM", "SOH_RestoreRandoSettings"})
otr = source("soh/soh/OTRGlobals.cpp")
settings = source("soh/soh/Enhancements/randomizer/settings.cpp")
option = source("soh/soh/Enhancements/randomizer/option.cpp")
extract = lambda text, signature: block_from(text, text.index(signature))
parts = ["namespace Rando {\n" + extract(settings, "std::shared_ptr<Settings> Settings::GetInstance()") + "\n" +
         extract(settings, "const std::array<Option, RSK_MAX>& Settings::GetAllOptions() const") + "\n" +
         extract(option, "const std::string& Option::GetCVarName() const") + "\n}",
         functions(otr)["Combo_ClearNumericExclusions"],
         soh.get("SOH_NormalizeComboGraceFromMM", "void SOH_NormalizeComboGraceFromMM(void) {}"),
         soh["SOH_RestoreRandoSettings"],
         functions(source("mm/2s2h/BenPort.cpp"), {"MM_RestoreRandoSettings"})["MM_RestoreRandoSettings"],
         functions(source("combo/ComboShip.cpp"), {"Combo_WriteMMSaveForSlot"})["Combo_WriteMMSaveForSlot"]]
with tempfile.TemporaryDirectory(prefix="mm-grace-settings-") as temporary:
    build = Path(temporary)
    (build / "settings_production.inc").write_text("\n".join(parts))
    binary = build / "settings"
    result = subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-w", *flags(),
        "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(build),
        "-include", "nlohmann/json.hpp", "-I" + str(ROOT), str(ROOT / "tests/mm_grace/settings_test.cpp"), "-o", str(binary)
    ], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary), *[arg for arg in sys.argv[1:] if arg != "--baseline"]], check=True)
