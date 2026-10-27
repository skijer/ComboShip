"""Check that Slate casing/extrusion faces form consistently wound closed solids."""
from collections import defaultdict
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / "SOURCE"))
from quest_revamp import RUNES, slate


def check_part(part):
    edges = defaultdict(list)
    for face in part["tri"]:
        points = [tuple(part["p"][index]) for index in face]
        for a, b in zip(points, points[1:] + points[:1]):
            edges[tuple(sorted((a, b)))].append(1 if a < b else -1)
    bad = [key for key, uses in edges.items() if len(uses) != 2 or sum(uses)]
    assert not bad, f'{part["name"]}: {len(bad)} open or inconsistently wound edges'


def main():
    for slug in RUNES:
        model = slate(slug)
        parts = [part for part in model.parts if part["name"] in
                 ("Thick chamfered ancient casing", "Handle mounting bracket")]
        assert len(parts) == 3
        for part in parts:
            check_part(part)
    print("PASS: all six Slate casings and handle mounts are consistently wound closed solids")


if __name__ == "__main__":
    main()
