"""Check the authored and serialized Time Gate body's closed, outward shell."""
from collections import Counter, defaultdict
from pathlib import Path
import sys
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "SOURCE"))
import meshkit
from meshkit import Q
from models import gate
from verify_assets import face_key, read_glb, verify


def main():
    model = gate()
    body, = [part for part in model.parts if part["name"] == "Carved gear body"]
    edges = defaultdict(list)
    for face in body["tri"]:
        points = [tuple(body["p"][index]) for index in face]
        for a, b in zip(points, points[1:] + points[:1]):
            edges[tuple(sorted((a, b)))].append(1 if a < b else -1)
    bad = [edge for edge, uses in edges.items() if len(uses) != 2 or sum(uses)]
    assert not bad, f"Time Gate body: {len(bad)} open or inconsistently wound edges"

    # A consistently closed shell can still face inward. Its signed volume
    # must be positive so back-face culling retains the exterior walls.
    triangles = body["p"][body["tri"]]
    volume = np.einsum("ij,ij->i", triangles[:, 0],
                       np.cross(triangles[:, 1], triangles[:, 2])).sum() / 6
    assert volume > 0, "Time Gate body has inward winding"

    # Prove that the check covers the triangles actually exported, including
    # both engravings and rims, instead of only an unshipped authoring model.
    authored = Counter(face_key(part["p"][face] * Q)
                       for part in model.parts for face in part["tri"])
    _, checkpoint = read_glb(ROOT / "CHECKPOINTS/time_gate/time_gate.glb")
    assert authored == checkpoint["opa"], "Time Gate authoring/checkpoint mismatch"
    verify(("time_gate",))
    with tempfile.TemporaryDirectory(prefix="time-gate-solidity-") as temp:
        # RESOURCES is an ignored build output. Regenerate it in scratch so
        # this regression also works from a clean checkout.
        original_root = meshkit.ROOT
        try:
            meshkit.ROOT = Path(temp)
            meshkit.export_resources(model)
        finally:
            meshkit.ROOT = original_root
        exported = Path(temp) / "RESOURCES"
        verify(("time_gate",), assets=exported)
        for path in (exported / model.prefix).iterdir():
            assert path.read_bytes() == (ROOT.parents[1] / "soh/assets/custom" / model.prefix / path.name).read_bytes(), path.name
    print("PASS Time Gate: closed outward body; fresh authoring export, GLB and shipped resources agree")


if __name__ == "__main__":
    main()
