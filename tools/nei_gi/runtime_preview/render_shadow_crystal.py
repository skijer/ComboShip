"""Preview the restored mesh with production charcoal/orange GI effects.

Camera and lighting are offline approximations, not captured gameplay.
The sampler uses the same 1/16-unit position packing as the real renderer.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import numpy as np
from PIL import ImageDraw
import render as r

REPO = Path(__file__).resolve().parents[3]
DTYPE = np.dtype([("p", "<f4", 3), ("rgba", "u1", 4)])


def export_effects(directory, frames):
    source = r'''
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <fstream>
int main(int argc, char** argv) {
    std::ofstream out(argv[1],std::ios::binary);
    for (unsigned frame=0; frame<FRAMES; ++frame) for (int view=0; view<2; ++view) {
        const float x=12*NeiGi::Tau/360, y=(25+(view ? 90 : 0))*NeiGi::Tau/360;
        const float sx=std::sin(x),cx=std::cos(x),sy=std::sin(y),cy=std::cos(y);
        NeiGi::Basis camera{{cy,0,sy},{sx*sy,cx,-sx*cy},{-cx*sy,sx,cx*cy}};
        auto energy=NeiGi::SampleSpecial(NeiGi::Kind::DarkCrystal,frame,camera);
        auto shimmer=NeiGi::SampleShimmer(frame,true,camera,NeiGi::Kind::DarkCrystal);
        uint32_t count=energy.count+shimmer.count;
        out.write(reinterpret_cast<char*>(&count),4);
        for (auto* mesh:{&energy,&shimmer}) for (size_t i=0;i<mesh->count;++i) {
            const auto& v=mesh->vertices[i];
            float p[]={std::round(v.p.x*16)/16,std::round(v.p.y*16)/16,std::round(v.p.z*16)/16};
            uint8_t rgba[]={uint8_t(v.rgb>>16),uint8_t(v.rgb>>8),uint8_t(v.rgb),v.alpha};
            out.write(reinterpret_cast<char*>(p),12);
            out.write(reinterpret_cast<char*>(rgba),4);
        }
    }
}
'''.replace("FRAMES", str(frames))
    cpp, binary, output = (directory / name for name in ("effects.cpp", "export", "effects.bin"))
    cpp.write_text(source)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2", "-I" + str(REPO / "soh"),
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary), str(output)], check=True)
    raw, offset, result = output.read_bytes(), 0, []
    for frame in range(frames):
        views = []
        for _ in range(2):
            count = struct.unpack_from("<I", raw, offset)[0]
            offset += 4
            views.append(np.frombuffer(raw, dtype=DTYPE, count=count, offset=offset))
            offset += count * 16
        result.append(views)
    assert offset == len(raw)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--frames", type=int, default=180)
    args = parser.parse_args()
    assert 1 <= args.frames <= 180
    args.output.mkdir(parents=True, exist_ok=True)
    r.W, r.H = 1200, 700
    opaque, _ = r.model("shadow_crystal")
    with tempfile.TemporaryDirectory(prefix="shadow-crystal-preview-") as temporary:
        effects = export_effects(Path(temporary), args.frames)
        movie = args.output / "Shadow_Crystal_Native_GI_POC1.mp4"
        command = ["ffmpeg", "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
                   "-s", f"{r.W}x{r.H}", "-r", "20", "-i", "-", "-an", "-c:v", "libx264",
                   "-preset", "fast", "-crf", "18", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(movie)]
        with subprocess.Popen(command, stdin=subprocess.PIPE) as encoder:
            for frame in range(args.frames):
                r.DepthMask(1)
                r.Clear(0x4000 | 0x0100)
                for view in range(3):
                    r.pose(view * 400, 95, 400, 540, -42, 42)
                    if view == 2:
                        r.Rotate(90, 0, 1, 0)
                    r.DepthMask(1)
                    r.CallList(opaque)
                    if view:
                        r.DepthMask(0)
                        r.drawfx(effects[frame][view - 1])
                image = r.pixels()
                draw = ImageDraw.Draw(image)
                draw.rectangle((0, 0, r.W, 66), fill="#111923")
                draw.text((24, 12), "Shadow Crystal · preserved black/orange mesh", font=r.font(26), fill="#eef3fd")
                draw.text((24, 47), "Candidate POC1 · exact mesh and production effect triangles · offline lighting", font=r.font(15), fill="#aabacc")
                for view, title in enumerate(("Mesh", "Charcoal + orange effects", "Side view")):
                    draw.text((view * 400 + 28, 645), title, font=r.font(22), fill="#eef3fd")
                if frame == min(36, args.frames - 1):
                    image.save(args.output / "Shadow_Crystal_Native_GI_POC1.png")
                encoder.stdin.write(image.tobytes())
            encoder.stdin.close()
            assert encoder.wait() == 0
        print(movie)


if __name__ == "__main__":
    main()
