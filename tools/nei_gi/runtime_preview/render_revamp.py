"""Render candidate GLBs with the real special-effect policy and exported GI centers.

Input comes from NEI_SHOP_PREVIEW_EXPORT in run_nei_gi_tests.py. Lighting and
camera are offline approximations; this is not game/runtime acceptance.
"""
import argparse
import json
import os
import struct
import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import ImageDraw
import render as r

SLUGS = (
    "sand_rod", "tornado_rod", "water_rod", "meteor_rod",
    "storm_rod", "shadow_scepter", "sheikah_slate", "phantom_hourglass",
    "shadow_crystal", "true_master_sword", "cane_of_byrna", "rod_of_seasons",
)
REPO = Path(__file__).resolve().parents[3]
DTYPE = np.dtype([("p", "<f4", 3), ("rgba", "u1", 4)])


def export_effects(items, frames, directory):
    profiles = ",".join("{" + str(i["nei_effect"]) + "," + str(i["always_shimmer"]).lower() + "}" for i in items)
    source = r'''
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <fstream>
int main(int argc, char** argv) {
    std::ofstream out(argv[1], std::ios::binary);
    const float x=12*NeiGi::Tau/360, y=25*NeiGi::Tau/360;
    const float sx=std::sin(x), cx=std::cos(x), sy=std::sin(y), cy=std::cos(y);
    NeiGi::Basis camera{{cy,0,sy},{sx*sy,cx,-sx*cy},{-cx*sy,sx,cx*cy}};
    struct Profile {int kind; bool shimmer;};
    const Profile profiles[] = { PROFILES };
    for (unsigned frame=0;frame<FRAMES;++frame) for (auto profile:profiles) {
        auto kind=static_cast<NeiGi::Kind>(profile.kind);
        auto mesh=NeiGi::SampleSpecial(kind,frame,camera);
        auto shimmer=NeiGi::SampleShimmer(frame,profile.shimmer,camera,kind);
        uint32_t count=mesh.count+shimmer.count;
        out.write(reinterpret_cast<char*>(&count),4);
        for (auto* m:{&mesh,&shimmer}) for (size_t i=0;i<m->count;++i) {
            auto v=m->vertices[i];
            float p[]={std::round(v.p.x*16)/16,std::round(v.p.y*16)/16,std::round(v.p.z*16)/16};
            uint8_t color[]={uint8_t(v.rgb>>16),uint8_t(v.rgb>>8),uint8_t(v.rgb),v.alpha};
            out.write(reinterpret_cast<char*>(p),12);
            out.write(reinterpret_cast<char*>(color),4);
        }
    }
}
'''.replace("PROFILES", profiles).replace("FRAMES", str(frames))
    cpp, binary, data = (directory / name for name in ("effects.cpp", "export", "effects.bin"))
    cpp.write_text(source)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2", "-I" + str(REPO / "soh"), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary), str(data)], check=True)
    raw, cursor, result = data.read_bytes(), 0, []
    for _ in range(frames):
        row = []
        for _ in items:
            count = struct.unpack_from("<I", raw, cursor)[0]
            cursor += 4
            row.append(np.frombuffer(raw, dtype=DTYPE, count=count, offset=cursor))
            cursor += count * 16
        result.append(row)
    assert cursor == len(raw)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("poses", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--frames", type=int, default=180)
    args = parser.parse_args()
    assert 1 <= args.frames <= 180
    args.output.mkdir(parents=True, exist_ok=True)
    descriptors = {i["slug"]: i for i in json.loads(args.poses.read_text())["items"]}
    items = [descriptors[slug] for slug in SLUGS]
    models = [r.model(slug) for slug in SLUGS]
    with tempfile.TemporaryDirectory(prefix="nei-revamp-preview-") as temporary:
        effects = export_effects(items, args.frames, Path(temporary))
        movie = args.output / "NEI_GI_Model_Effects_Candidate.mp4"
        command = ["ffmpeg", "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{r.W}x{r.H}", "-r", "20", "-i", "-", "-an", "-c:v", "libx264", "-preset", "fast", "-crf", "18", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(movie)]
        with subprocess.Popen(command, stdin=subprocess.PIPE) as encoder:
            for frame in range(args.frames):
                r.DepthMask(1)
                r.Clear(0x4000 | 0x0100)
                for index, ((opaque, skin), info) in enumerate(zip(models, items)):
                    x, y = (index % 4) * 300, 90 + (2 - index // 4) * 285
                    r.pose(x, y, 300, 255, -45, 55)
                    r.DepthMask(1)
                    r.CallList(opaque)
                    r.DepthMask(0)
                    r.Push()
                    r.Translate(*info["effect_center"])
                    r.drawfx(effects[frame][index])
                    r.Pop()
                    r.CallList(skin)
                image = r.pixels()
                draw = ImageDraw.Draw(image)
                draw.rectangle((0, 0, r.W, 70), fill="#111923")
                draw.text((18, 12), "NEI GI model and effect candidates", font=r.font(27), fill="#edf2f8")
                draw.text((18, 49), "Production effect triangles + exact candidate GLBs · offline camera and lighting", font=r.font(16), fill="#aab9c8")
                for index, info in enumerate(items):
                    draw.text(((index % 4) * 300 + 10, 348 + (index // 4) * 285), info["name"], font=r.font(16), fill="#edf2f8")
                if frame == min(36, args.frames - 1):
                    image.save(args.output / "NEI_GI_Model_Effects_Candidate.png")
                encoder.stdin.write(image.tobytes())
            encoder.stdin.close()
            assert encoder.wait() == 0
        print(movie)


if __name__ == "__main__":
    main()
