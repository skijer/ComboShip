"""Load the packaged generated leaves through the native texture factory."""
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ["tests/seasons/generated_leaf_textures_test.cpp",
           "libultraship/src/fast/resource/factory/TextureFactory.cpp",
           "libultraship/src/fast/resource/type/Texture.cpp",
           "libultraship/src/ship/resource/ResourceFactoryBinary.cpp",
           "libultraship/src/ship/resource/Resource.cpp",
           "libultraship/src/ship/utils/binarytools/BinaryReader.cpp",
           "libultraship/src/ship/utils/binarytools/MemoryStream.cpp",
           "libultraship/src/ship/utils/binarytools/Stream.cpp"]
with tempfile.TemporaryDirectory(prefix="autumn-generated-textures-") as temporary:
    binary = Path(temporary) / "textures"
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-DFMT_HEADER_ONLY",
                             "-Ilibultraship/include", *SOURCES, "-o", str(binary)],
                            cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary), str(ROOT)], check=True)
