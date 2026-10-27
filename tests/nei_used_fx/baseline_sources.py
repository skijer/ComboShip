"""Read exact published comparison sources independently of clone depth."""
import hashlib
import json
from pathlib import Path

BASELINE = Path(__file__).resolve().parent / "baseline"
SOURCES = json.loads((BASELINE / "sources.json").read_text())["sources"]


def read_source(commit, path):
    record = SOURCES[commit + ":" + path]
    data = (BASELINE / record["file"]).read_bytes()
    identity = b"blob " + str(len(data)).encode() + b"\0" + data
    if hashlib.sha1(identity).hexdigest() != record["git_blob_sha"]:
        raise ValueError("Historical baseline blob changed: " + commit + ":" + path)
    return data.decode("utf-8")

