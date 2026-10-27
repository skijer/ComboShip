"""UV sampling modes for the reviewed 512-tile native NPC emit_mesh state."""
import numpy as np


def native_uv_modes(uv):
    """Match GfxSpVertex's 0xffff scale and GfxSpTri's filter half-texel.

    The reviewed lists encode signed ST as normalized UV times 16384, use
    a 512x512 logical tile with zero origin/shifts, and set texture scale65535.
    Returned normalized shader UVs cover point and linear filtering. Raster
    protection must additionally include the relevant sampling kernel.
    """
    uv = np.asarray(uv, float)
    if uv.shape[-1] != 2 or not np.isfinite(uv).all():
        raise ValueError('Expected finite normalized NPC UV coordinates')
    raw_st = np.rint(uv*16384).astype(np.int64)
    point = ((raw_st*65535) >> 16).astype(float)/16384
    return point, point+.5/512
