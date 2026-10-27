"""Restore source-authentic HD images into the privately copied Sages center.

The MAP donor embeds low-resolution textures. Once its resources are copied to
our private namespace, the separate Djipi HD pack's original paths no longer
override them. Bundle the three unchanged HD resources and retain their raw
upload metadata, logical sampler dimensions, material state, and atlas UVs.
"""
import hashlib
import json
from pathlib import Path


ASSETS = Path(__file__).resolve().parent / 'assets/chamber_hd'
TEXTURES = ('rm_kja_water_02_13C00', 'rm_kja_mark_01_C000', 'rm_kja_yuka_01_16C00')


def apply(resources, namespace='custom/temple_sages/chamber/'):
    """Return the resource graph with only the three private textures restored.

    Canonical aliases make the same resources available to hash lookup before
    Alt resolution. The caller's graph and every non-target resource are kept
    unchanged; neither a separate HD pack nor donor archive is needed here.
    """
    provenance = json.loads((ASSETS / 'provenance.json').read_text())
    result = dict(resources)
    for name in TEXTURES:
        source = provenance['textures'][name]
        data = (ASSETS / source['file']).read_bytes()
        if hashlib.sha256(data).hexdigest() != source['resource_sha256']:
            raise ValueError('HD chamber source resource changed: ' + name)
        result[namespace + name] = data
        result['alt/' + namespace + name] = data
    return result
