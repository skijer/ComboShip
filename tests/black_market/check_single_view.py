#!/usr/bin/env python3
"""The exported preview must use one complete scene view per video frame."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
from preview import export_video, gpu

calls = []
def scene(**options):
    calls.append(options)
    image = Image.new("RGB", (gpu.W, gpu.H), "#3a293d")
    image.info["stock_markers"] = []
    image.info["clears"] = 0
    return image

with tempfile.TemporaryDirectory(prefix="market-single-view-") as temporary:
    video = Path(temporary) / "single-view.mp4"
    export_video(scene, [], video, 2)
    assert len(calls) == 2, "each output frame must render exactly one scene, not three pasted camera views"
    assert [c["tick"] for c in calls] == [0, 3]
    assert calls[0]["view"] == calls[1]["view"] and calls[0]["fov"] == calls[1]["fov"]
    probe = json.loads(subprocess.check_output([
        "ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
        "stream=width,height,nb_frames", "-of", "json", str(video)], text=True))["streams"][0]
    assert (probe["width"], probe["height"]) == (gpu.W, gpu.H), "keep the complete camera image without stitched crops"
    assert int(probe["nb_frames"]) == 2
print("PASS preview video: one continuous scene/camera per frame, unchanged aspect and complete viewport")
