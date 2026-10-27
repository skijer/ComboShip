"""Package the optional Cojiro GI without writing built-in game assets."""
import argparse
from pathlib import Path
import zipfile

from verify_assets import OPTIONAL_ASSETS, PREFIX, verify


def package(output):
    verify(("cojiro",), assets=OPTIONAL_ASSETS)
    sources = {p.relative_to(OPTIONAL_ASSETS).as_posix(): p.read_bytes()
               for p in sorted((OPTIONAL_ASSETS / PREFIX / "cojiro").iterdir()) if p.is_file()}
    assert len(sources) == 3
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path, data in sources.items():
            for name in (path, "alt/" + path):
                info = zipfile.ZipInfo(name, (2026, 10, 4, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o644 << 16
                archive.writestr(info, data)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == len(sources) * 2
        assert set(archive.namelist()) == set(sources) | {"alt/" + p for p in sources}
        for path, data in sources.items():
            assert archive.read(path) == archive.read("alt/" + path) == data
    print("PASS: optional Cojiro archive; 3 exact resources, matching base/Alt entries, GLB geometry and references")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    package(parser.parse_args().output)
