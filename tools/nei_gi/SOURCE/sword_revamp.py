"""Candidate reconstructions of the eight progressive weapon GI presentations.

The source checkout records the original GI resource identity and tier routing;
the Iron Knuckle axe also has its original inline vertex/display-list data.
These are newly authored NEI-style meshes, not recovered Nintendo source models.
Only the new GI namespaces are exported. Held weapons and the Din sword pack
remain owned by their existing draw paths.
"""
import argparse
import math
import numpy as np
from meshkit import Model, TAU, bezier, earclip, rotation, unit


def _model(slug, name):
    m = Model(slug, name, 'objects/nei_gi_redesign/' + slug + '/gi_dl', 1., .65)
    m.material('steel', [.69, .76, .80], 'metal', .85, .29)
    m.material('steel_shadow', [.40, .48, .55], 'metal', .8, .38)
    m.material('blade_edge', [.92, .96, .98], 'metal', .88, .22)
    m.material('gold', [.77, .48, .13], 'metal', .82, .34)
    m.material('gold_edge', [.98, .76, .32], 'metal', .88, .25)
    m.material('dark_steel', [.13, .17, .20], 'metal', .7, .42)
    m.material('wood', [.38, .19, .075], 'wood', 0., .73)
    m.material('ruby', [.67, .035, .055], None, .45, .19)
    return m


def _faces(m, name, mat, positions, triangles, uv=None):
    """Each hard surface has its exact geometric normal on both sides."""
    p = np.asarray(positions, float)
    t = np.asarray(triangles, int).reshape(-1, 3)
    pp = p[t].reshape(-1, 3)
    normal = np.repeat(unit(np.cross(p[t[:, 1]] - p[t[:, 0]],
                                    p[t[:, 2]] - p[t[:, 0]])), 3, axis=0)
    if uv is None:
        uv = np.c_[p[:, 0] / 22 + .5, p[:, 1] / 55 + .5]
    uv = np.asarray(uv)[t].reshape(-1, 2)
    m.add(name, mat, pp, normal, uv, np.arange(len(pp)).reshape(-1, 3))


def _blade(m, name, profile, depth=3.8, core='steel', edge='blade_edge',
           ridge='steel_shadow'):
    """Closed eight-facet section, with a real ridge and broad cutting bevels.

    profile consists of (y, half-width[, centre-x]); edge vertices meet at z=0.
    Opposite facets and end caps give the back the same construction as front.
    """
    rings = []
    section = [(-1, 0), (-.66, .62), (0, 1), (.66, .62), (1, 0),
               (.66, -.62), (0, -1), (-.66, -.62)]
    for row in profile:
        y, width = row[:2]
        cx = row[2] if len(row) == 3 else 0
        rings.append([[cx + width*x, y, depth*.5*z] for x, z in section])
    rings = np.asarray(rings)
    for j in range(8):
        p = []
        for ring in rings:
            p += [ring[j].tolist(), ring[(j+1) % 8].tolist()]
        tri = []
        for i in range(len(rings)-1):
            a = i*2
            tri += [[a, a+1, a+2], [a+1, a+3, a+2]]
        material = edge if j in (0, 3, 4, 7) else (ridge if j in (2, 6) else core)
        _faces(m, name + ' facet %d' % j, material, p, tri)
    for i in (0, len(rings)-1):
        p = rings[i]
        tri = [[0, j, j+1] for j in range(1, 7)]
        if i == 0:
            tri = np.asarray(tri)[:, ::-1]
        _faces(m, name + ' terminal', core, p, tri)


def _beveled_plate(m, name, outline, face='steel', edge='blade_edge', depth=4.):
    """A closed concave plate with distinct broad, sharpened edge facets."""
    xy = np.asarray(outline, float)
    center = xy.mean(axis=0)
    inside = center + (xy-center)*.78
    count = len(xy)
    for sign in (1, -1):
        p = np.c_[inside, np.full(count, sign*depth*.5)]
        tri = np.asarray(earclip(inside))
        if sign < 0:
            tri = tri[:, ::-1]
        _faces(m, name + ' face', face, p, tri)
        p = np.vstack([np.c_[inside, np.full(count, sign*depth*.5)],
                       np.c_[xy, np.full(count, sign*.28)]])
        tri = []
        for j in range(count):
            k = (j+1) % count
            tri += [[j, k+count, k], [j, j+count, k+count]]
        if sign < 0:
            tri = np.asarray(tri)[:, ::-1]
        _faces(m, name + ' broad bevel', edge, p, tri)
    p = np.vstack([np.c_[xy, np.full(count, .28)],
                   np.c_[xy, np.full(count, -.28)]])
    tri = []
    for j in range(count):
        k = (j+1) % count
        tri += [[j, k+count, k], [j, j+count, k+count]]
    _faces(m, name + ' cutting rim', edge, p, tri)


def _band(m, name, material, y, radius, height=.7, sides=12):
    m.lathe(name, material, [(y-height, radius*.86), (y-height*.65, radius),
                             (y+height*.65, radius), (y+height, radius*.86)], sides)


def _grip(m, bottom, top, radius, color, fittings='gold', wrap=True):
    m.material('grip', color, 'leather', 0, .76)
    m.material('grip_seam', (np.asarray(color)*1.38).clip(0, 1).tolist(), 'leather', 0, .68)
    m.lathe('Textured leather grip', 'grip', [(bottom, radius*.87),
              (bottom+1.2, radius), (top-1.2, radius), (top, radius*.9)], 12)
    for y in (bottom+.5, top-.5):
        _band(m, 'Grip metal ferrule', fittings, y, radius+.45, .7)
    if wrap:
        t = np.linspace(0, 1, 50)
        a = t*TAU*5.2
        r = radius+.08
        m.tube('Raised spiral leather binding', 'grip_seam',
               np.c_[np.cos(a)*r, bottom+1.8+t*(top-bottom-3.6), np.sin(a)*r], .24, 4)


def _gem(m, name, y, radius=2.3, mat='ruby', z=2.3):
    for sign in (1, -1):
        m.sphere(name + ' setting', 'gold_edge', [0, y, sign*z],
                 [radius+1., radius+1.2, .8], 5, 10)
        m.crystal(name, mat, [0, y, sign*(z+.7)], radius, radius*2.3, 6)


def _triforce(m, y, z, size=4., material='gold_edge'):
    h = size*math.sqrt(3)/2
    for sign in (1, -1):
        for x, yy in [(-size/2, y), (size/2, y), (0, y+h)]:
            m.polygon('Sacred Triforce crest', material,
                      [(x-size*.46, yy-h*.32), (x+size*.46, yy-h*.32),
                       (x, yy+h*.67)], .20, sign*z, .08)


def _finish(m, lean=-15):
    positions = np.concatenate([p['p'] for p in m.parts])
    midpoint = (positions.min(axis=0) + positions.max(axis=0))/2
    m.transform(offset=-midpoint)
    m.transform(rotation('z', lean))
    # Re-quantizing the baked presentation can reverse a sub-texel bevel or
    # tapered leaf cap. Apply the same winding/normal agreement as Model.add
    # after the final transform, before the GLB and resource export diverge.
    for part in m.parts:
        p, n, tri = part['p'], part['n'], part['tri']
        faces = np.cross(p[tri[:, 1]]-p[tri[:, 0]], p[tri[:, 2]]-p[tri[:, 0]])
        flip = np.einsum('ij,ij->i', faces, n[tri].sum(axis=1)) < 0
        tri[flip] = tri[flip][:, [0, 2, 1]]
    m.markers.update(presentation_only=True, draw_scale=1., effective_author_scale=.65,
                     authored_lean_degrees=lean, reconstruction=True)
    m.notes += ['New candidate reconstruction; source declarations and original tier identities were retained.',
                'GI-only namespace; held meshes, save progression, upgrade flame, and Din sword resources are unchanged.',
                'Opaque beveled geometry on both faces; author units are quantized by the shared exporter.']
    return m


def kokiri_sword():
    m = _model('kokiri_sword', 'Kokiri Sword')
    _blade(m, 'Short broad Kokiri steel blade', [(-12, 5.5), (-9, 7.2),
           (28, 6.4), (39, 5.4), (50, 0)], 3.4)
    # The compact gold guard, red grip, and red centre gem retain OoT identity.
    guard = [(-18, -15.5), (-17, -10.5), (-10, -9.4), (-5, -11.5),
             (0, -10), (5, -11.5), (10, -9.4), (17, -10.5),
             (18, -15.5), (11, -14.8), (4, -15.5), (-4, -15.5), (-11, -14.8)]
    m.polygon('Compact gold Kokiri crossguard', 'gold', guard, 5.2, 0, .8)
    for sign in (1, -1):
        m.tube('Guard bright rolled rim', 'gold_edge',
               [[-16, -11.4, sign*2.5], [-10, -10.4, sign*2.6],
                [0, -12, sign*2.65], [10, -10.4, sign*2.6], [16, -11.4, sign*2.5]], .32, 5)
    _grip(m, -36.5, -16, 3.2, [.43, .055, .035])
    m.lathe('Kokiri gold pommel', 'gold', [(-46, 1.8), (-44.6, 4.),
              (-41, 4.7), (-38.2, 3.6), (-36.5, 3.4)], 12)
    _band(m, 'Pommel polished gold band', 'gold_edge', -41.9, 4.65, .6)
    _gem(m, 'Kokiri red guard gem', -12.7, 2.0, z=2.7)
    m.polygon('Gold ricasso collar', 'gold', [(-4.2, -12), (4.2, -12),
              (3.5, -7), (-3.5, -7)], 3.7, 0, .4)
    m.notes = ['Short leaflike silver blade, compact gold guard, brick-red leather and red guard stone.',
               'Base of the Kokiri > Razor > Gilded chain.']
    return _finish(m, -18)


def razor_sword():
    m = _model('razor_sword', 'Razor Sword')
    # The decoded public held model is a secondary silhouette reference, not
    # original GI vertex data. Its asymmetric hooked edge, shorter lower spur
    # and three connecting bridges are one blade assembly, not two free tines.
    # Preserve this GI reconstruction's 74-unit exposed blade height while
    # transferring the reference XY silhouette into the upright blade plane.
    held_scale = 74 / 1576
    def held_outline(points, ccw=True):
        outline = [((y-231.5)*held_scale, (x-326)*held_scale-12)
                   for x, y in points]
        # Swapping the held axes reverses winding. _beveled_plate requires CCW
        # contours, while Model.polygon's side facets require CW contours.
        area = sum(a[0]*b[1]-b[0]*a[1]
                   for a, b in zip(outline, outline[1:]+outline[:1]))
        return outline if (area > 0) == ccw else outline[::-1]
    upper = held_outline([(326, 266), (1514, 267), (1660, 111),
                          (1902, 122), (1771, 298), (1496, 431), (326, 432)])
    lower = held_outline([(326, 31), (1330, 37), (1452, 183), (326, 182)])
    _beveled_plate(m, 'Razor hooked broad cutting edge', upper, depth=3.2)
    _beveled_plate(m, 'Razor shorter clipped cutting spur', lower, depth=3.2)
    for i, (start, end) in enumerate(((326, 414), (790, 878), (1244, 1333))):
        bridge = held_outline([(start, 178), (end, 178), (end, 271), (start, 271)],
                              ccw=False)
        m.polygon('Razor integral steel bridge %d' % (i+1), 'steel',
                  bridge, 1.5, 0, .18)
    _blade(m, 'Razor seated blade tang', [(-17, 3.1), (-12, 7.)], 3.2)
    m.material('plum_enamel', [.24, .12, .31], 'ceramic', .5, .31)
    guard = [(-20, -17), (-23, -11), (-19, -4), (-17, -10), (-8, -12),
             (-5, -9.5), (0, -12), (5, -9.5), (8, -12), (17, -10),
             (19, -4), (23, -11), (20, -17), (10, -16), (-10, -16)]
    m.polygon('Upswept silver Razor guard', 'steel_shadow', guard, 5.2, 0, .7)
    for sign in (1, -1):
        m.tube('Razor guard polished upper edge', 'blade_edge',
               [[-19, -7, sign*2.55], [-17, -11, sign*2.55], [-8, -13, sign*2.55],
                [0, -12, sign*2.55], [8, -13, sign*2.55], [17, -11, sign*2.55],
                [19, -7, sign*2.55]], .38, 5)
    _grip(m, -39, -17, 3.1, [.24, .065, .15], 'steel_shadow')
    m.crystal('Razor faceted silver pommel', 'steel', [0, -45, 0], 4.7, 12, 8)
    _band(m, 'Razor pommel plum collar', 'plum_enamel', -40.4, 4.1, .9)
    for sign in (1, -1):
        m.polygon('Plum Razor central shield', 'plum_enamel',
                  [(-3.8, -15), (3.8, -15), (2.4, -9.6), (0, -7.8), (-2.4, -9.6)],
                  .5, sign*2.8, .15)
        m.polygon('Razor central silver inset', 'blade_edge',
                  [(-1.7, -13), (1.7, -13), (0, -9.5)], .18, sign*3.2, .07)
    m.notes = ['Connected asymmetric steel Razor blade with a hooked upper edge, shorter clipped spur and three bridges across its narrow slot; silver guard and plum/red leather grip retained.',
               'Silhouette follows decoded public Z64Online held Razor geometry as a secondary identity reference; original Nintendo GI vertices and absolute GI dimensions were unavailable.',
               'The secondary held blade was rescaled from 1576 long-axis units to the existing 74-unit GI blade span; upright centered presentation is a reviewable reconstruction.']
    m = _finish(m, 0)
    m.markers.update(guard_baseline_y=-17.5*m.native_scale*16,
                     proportion_reference='secondary held Razor silhouette; GI reconstruction candidate',
                     secondary_reference='https://github.com/hylian-modding/Z64Online/blob/d1c59355532d7fd81209a40e73dbf586a4738aa9/src/Z64Online/mm/models/zobjs/gear.ts',
                     original_gi_vertex_data_available=False)
    return m


def gilded_sword():
    m = _model('gilded_sword', 'Gilded Sword')
    _blade(m, 'Gilded silver framed blade', [(-12, 6.8), (-6, 8.6),
           (48, 8.), (56, 5.8), (66, 0)], 3.5, ridge='steel')
    # Gold is exposed in three raised diamonds, with a silver metal frame.
    for sign in (1, -1):
        for y, w, height in [(5, 6.4, 19), (25, 6.3, 19), (45, 5.9, 18)]:
            m.polygon('Diamond dark inset border', 'dark_steel',
                      [(0, y-height*.54), (w+.5, y), (0, y+height*.54), (-w-.5, y)],
                      .28, sign*1.95, .1)
            m.polygon('Gilded blade golden diamond', 'gold_edge',
                      [(0, y-height*.45), (w-.3, y), (0, y+height*.45), (-w+.3, y)],
                      .32, sign*2.14, .13)
            m.polygon('Golden diamond satin centre', 'gold',
                      [(0, y-height*.35), (w*.67, y), (0, y+height*.35), (-w*.67, y)],
                      .13, sign*2.36, .05)
    guard = [(-23, -15), (-24, -8), (-20, -2.3), (-18, -8), (-12, -9.4),
             (-6, -11), (0, -9), (6, -11), (12, -9.4), (18, -8),
             (20, -2.3), (24, -8), (23, -15), (14, -14), (7, -15.5),
             (0, -14.5), (-7, -15.5), (-14, -14)]
    m.polygon('Gilded ornate silver crossguard', 'steel', guard, 5.5, 0, .8)
    for side in (-1, 1):
        for sign in (1, -1):
            m.tube('Silver quillon relief', 'blade_edge',
                   [[side*7, -12.2, sign*2.8], [side*15, -11, sign*2.8],
                    [side*19, -9, sign*2.8], [side*20, -5, sign*2.6]], .4, 5)
    _grip(m, -40, -17, 3.2, [.43, .025, .045], 'steel', wrap=False)
    for sign in (1, -1):
        for y in (-23, -33.8):
            m.polygon('Grip exposed gold diamond', 'gold_edge',
                      [(0, y-4.5), (2.7, y), (0, y+4.5), (-2.7, y)],
                      .2, sign*3.22, .08)
    m.material('white_pommel', [.87, .91, .90], 'metal', .8, .24)
    m.crystal('Gilded white faceted pommel', 'white_pommel', [0, -46, 0], 5.2, 12, 8)
    _band(m, 'Gilded pommel silver bezel', 'steel_shadow', -40.6, 4.6, .75)
    _gem(m, 'Gilded warm centre gold stone', -11.4, 2.2, mat='gold_edge', z=2.85)
    m.notes = ['Three golden blade diamonds framed by silver/dark metal; red grip diamonds and white pommel.',
               'Ornate upswept silver crossguard retains Gilded identity instead of recoloring a Master blade.']
    return _finish(m, -12)


def _master(awakened=False):
    slug = 'true_master_sword' if awakened else 'master_sword'
    m = _model(slug, 'True Master Sword' if awakened else 'Master Sword')
    m.material('sacred_blue', [.12, .13, .38], 'ceramic', .55, .32)
    m.material('blue_edge', [.28, .30, .68], 'metal', .72, .28)
    m.material('sacred_teal', [.14, .44, .43], 'leather', 0, .74)
    m.material('sacred_cyan', [.45, .79, .92], 'metal', .68, .26, emission=.12)
    tip = 67 if awakened else 61
    _blade(m, 'Sacred ridged silver blade', [(-12, 6.1), (-9, 7.6),
           (35, 7.0), (tip-13, 5.9), (tip, 0)], 3.7 if awakened else 3.4,
           ridge='steel')
    # The purple-blue winged guard is shared by the sacred family.
    guard = [(-25, -18), (-24, -10), (-19, -5), (-13, -3.5), (-9, -9),
             (-4, -11), (0, -9), (4, -11), (9, -9), (13, -3.5),
             (19, -5), (24, -10), (25, -18), (19, -15.5),
             (13, -13.2), (8, -13.7), (4, -17), (-4, -17),
             (-8, -13.7), (-13, -13.2), (-19, -15.5)]
    m.polygon('Master Sword blue winged guard', 'sacred_blue', guard, 5.8, 0, .85)
    for sign in (1, -1):
        for side in (-1, 1):
            for x in (12, 17, 21):
                path = [[side*x, -6.2-(x-12)*.48, sign*2.9],
                        [side*(x+1.4), -10.8-(x-12)*.36, sign*3.05],
                        [side*(x+.4), -14.1-(x-12)*.2, sign*2.9]]
                m.tube('Wing molded feather ridge', 'blue_edge', path, .38, 5)
    _grip(m, -38.8, -18, 3.05, [.10, .30, .31], 'sacred_blue', wrap=False)
    # Recognizable crossed blue grip straps, rather than a plain cylindrical hilt.
    t = np.linspace(0, 1, 31)
    for direction in (-1, 1):
        a = direction*t*TAU*2.5
        m.tube('Crossed Master grip straps', 'sacred_blue',
               np.c_[np.cos(a)*3.18, -37.5+t*18.4, np.sin(a)*3.18], .34, 5)
    m.lathe('Blue faceted Master pommel', 'sacred_blue', [(-49, 0),
              (-47.5, 3.1), (-43.2, 4.6), (-40, 3.7), (-38.5, 3.05)], 8)
    _band(m, 'Master pommel blue ridge', 'blue_edge', -43.6, 4.45, .7, 8)
    _gem(m, 'Master guard golden stone', -12, 2.55, mat='gold_edge', z=3.)
    _triforce(m, -2, 1.90 if awakened else 1.76, 3.2)
    # A short raised ricasso borders the crest without covering the silver blade.
    for sign in (1, -1):
        for side in (-1, 1):
            m.tube('Sacred crest silver border', 'blade_edge',
                   [[side*5.0, -8, sign*1.33], [side*5.3, -1.5, sign*1.32],
                    [side*3.4, 6.5, sign*1.60]], .24, 5)
    if awakened:
        # Upgrade construction, not a hue swap: longer blade, fine gold wing
        # outlines, extended sacred inlay, and a crowned blue pommel.
        for sign in (1, -1):
            for side in (-1, 1):
                m.tube('Awakened gold wing outline', 'gold_edge',
                       [[side*4, -12, sign*3.1], [side*9, -10.2, sign*3.1],
                        [side*13, -5, sign*2.9], [side*18, -6.2, sign*2.9],
                        [side*23, -11, sign*2.9]], .32, 5)
                m.tube('Awakened blade gold channel', 'gold',
                       [[side*4.1, 7, sign*1.47], [side*3.5, 18, sign*1.5],
                        [side*2.4, 30, sign*1.64], [side*.6, 40, sign*1.89]], .21, 5)
            m.polygon('Awakened sacred blue blade focus', 'sacred_cyan',
                      [(0, 9), (1.3, 12), (0, 17), (-1.3, 12)], .18, sign*2., .06)
        _band(m, 'Awakened gold pommel crown', 'gold', -44, 4.55, .8, 8)
        _band(m, 'Awakened lower grip gold ferrule', 'gold_edge', -37.8, 3.48, .42, 12)
    m.notes = ['Sacred purple-blue winged guard, crossed teal/blue grip, gold centre stone, Triforce ricasso.',
               ('Awakened tier adds a longer blade, modeled gold channels/wing outlines and crowned pommel; sacred silhouette retained.'
                if awakened else 'Base sacred blade preserves recognizable Master Sword construction and palette.'),
               'The existing True Master upgrade flame is a separate root-owned renderer effect.']
    return _finish(m, -17)


def master_sword():
    return _master(False)


def true_master_sword():
    return _master(True)


def biggoron_sword():
    m = _model('biggoron_sword', "Biggoron's Sword")
    # A greatsword reconstruction candidate: the 161-unit overall span
    # is longer than the Master Sword, and the 18-unit cutting width no longer
    # makes the blade read as a short cleaver. The short tang seats in the guard.
    # These are authored proportions, not recovered Nintendo GI coordinates.
    _blade(m, 'Long Biggoron steel blade and seated tang', [(-14, 3.1),
           (-10, 7.), (-6, 9.), (78, 8.4), (94, 7.2), (106, 0)], 3.6,
           ridge='steel')
    m.material('goron_bronze', [.68, .32, .07], 'metal', .8, .39)
    m.material('bronze_edge', [.95, .57, .20], 'metal', .82, .27)
    m.material('blue_grip', [.12, .17, .35], 'leather', 0, .76)
    guard = [(-19.2, -15), (-20, -8), (-15.8, -5), (-10, -7), (-5.8, -8.2),
             (0, -6.8), (5.8, -8.2), (10, -7), (15.8, -5), (20, -8),
             (19.2, -15), (12.5, -16), (5.8, -13.5), (-5.8, -13.5), (-12.5, -16)]
    m.polygon('Orange-bronze Biggoron crossguard', 'goron_bronze', guard, 5.2, 0, .75)
    for sign in (1, -1):
        m.tube('Biggoron solid guard rim', 'bronze_edge',
               [[-18.4, -11, sign*2.65], [-12.5, -9.9, sign*2.65], [0, -10, sign*2.65],
                [12.5, -9.9, sign*2.65], [18.4, -11, sign*2.65]], .36, 6)
        m.polygon('Goron guard stamped diamond', 'gold_edge',
                  [(0, -15), (4., -10.4), (0, -6), (-4., -10.4)], .23, sign*2.76, .15)
        m.polygon('Goron guard dark central mark', 'goron_bronze',
                  [(0, -12.9), (2.2, -10.4), (0, -7.9), (-2.2, -10.4)], .13, sign*2.94, .1)
    _grip(m, -48, -14, 3.2, [.115, .16, .31], 'goron_bronze')
    # A long two-hand grip and heavy disc pommel carry the weight of the blade.
    m.lathe('Biggoron heavy bronze pommel', 'goron_bronze',
            [(-55, 2.2), (-53.6, 4.), (-51.2, 4.7), (-48.8, 3.8), (-48, 3.4)], 12)
    _band(m, 'Biggoron pommel thick raised rim', 'bronze_edge', -51.2, 4.65, .65)
    for sign in (1, -1):
        m.tube('Biggoron ricasso reinforced ridge', 'steel_shadow',
               [[-5.1, -8, sign*1.20], [-4.9, 4, sign*1.28],
                [-2.5, 11, sign*1.56]], .23, 5)
        m.tube('Biggoron ricasso reinforced ridge', 'steel_shadow',
               [[5.1, -8, sign*1.20], [4.9, 4, sign*1.28],
                [2.5, 11, sign*1.56]], .23, 5)
    m.notes = ['Visual reconstruction candidate with a long steel blade, orange-bronze guard, 34-unit indigo two-hand grip and bronze pommel.',
               '161-unit full span, 18-unit blade width and 40-unit guard follow the proportion review; they are not recovered original Nintendo GI measurements.',
               'Upright centered GI presentation retains the bronze/indigo/steel material palette.']
    m = _finish(m, 0)
    m.markers.update(guard_baseline_y=-35.875*m.native_scale*16,
                     proportion_reference='proportion correction candidate from user feedback',
                     original_gi_vertex_data_available=False)
    return m


def _leaf(m, name, xy, z, sign=1, color='leaf_green', depth=.9):
    xy = np.asarray(xy, float)
    m.polygon(name, color, xy, depth, sign*z, .3)
    start = xy[0]
    end = xy[len(xy)//2]
    line = np.array([start, (start+end)/2, end])
    m.tube(name + ' modeled central vein', 'leaf_edge',
           np.c_[line, np.full(3, sign*(z+depth*.5+.08))], .19, 4)


def great_fairy_sword():
    m = _model('great_fairy_sword', "Great Fairy's Sword")
    m.material('rose_steel', [.82, .42, .63], 'metal', .76, .31)
    m.material('rose_ridge', [.57, .22, .45], 'metal', .72, .4)
    m.material('rose_edge', [.98, .70, .83], 'metal', .8, .24)
    m.material('leaf_green', [.16, .39, .13], 'leaf', .1, .63)
    m.material('leaf_edge', [.49, .65, .20], 'leaf', .1, .52)
    m.material('rose_black', [.085, .037, .082], None, .35, .54)
    m.material('bud_plum', [.35, .075, .23], 'ceramic', .38, .3)
    _blade(m, 'Broad pink Fairy blade', [(-11, 8), (-7, 9.8),
           (41, 10.), (52, 9.2), (68, 0)], 4.0,
           'rose_steel', 'rose_edge', 'rose_ridge')
    _grip(m, -39, -17, 3.1, [.15, .31, .115], 'gold')
    # The leaf guard spreads as living foliage, not a metal sword crossbar.
    for side in (-1, 1):
        leaf = np.array([[3, -12], [8, -5], [16, -2], [23, -5], [28, -11],
                         [19, -10.5], [16, -15], [10, -18], [6, -16]])
        leaf[:, 0] *= side
        m.polygon('Fairy broad leaf quillon', 'leaf_green', leaf, 4.2, 0, .65)
        for sign in (-1, 1):
            m.tube('Leaf quillon branching vein', 'leaf_edge',
                   [[side*4, -13, sign*2.3], [side*11, -10, sign*2.3],
                    [side*18, -7, sign*2.3], [side*26, -10, sign*2.15]], .29, 5)
            for x, y in [(10, -10), (16, -8.1), (21, -8.8)]:
                m.tube('Leaf short branch vein', 'leaf_edge',
                       [[side*x, y, sign*2.28], [side*(x+1.2), y-3, sign*2.28]], .17, 4)
    m.lathe('Fairy bud pommel mount', 'gold', [(-47, 2.8), (-43, 4.1),
              (-40, 3.4), (-38.5, 3.1)], 10)
    m.sphere('Fairy dark rose bud pommel', 'bud_plum', [0, -48, 0], [4.8, 5.7, 4.8], 7, 12)
    for j in range(5):
        a = TAU*j/5
        path = [[3.6*math.cos(a), -42, 3.6*math.sin(a)],
                [4.75*math.cos(a), -46.5, 4.75*math.sin(a)],
                [2.6*math.cos(a+.4), -51.5, 2.6*math.sin(a+.4)]]
        m.tube('Pommel curling leaf sepal', 'leaf_green', path, [.65, .8, .16], 5)
    # A dark rose relief and green branching vine lie on each blade face.
    for sign in (-1, 1):
        m.sphere('Fairy green blade medallion', 'leaf_green', [0, 3, sign*2.12],
                 [5.4, 7.2, .34], 6, 16)
        for j in range(5):
            a = TAU*j/5
            m.sphere('Dark rose emblem petal', 'rose_black',
                     [math.sin(a)*2.2, 3+math.cos(a)*2.2, sign*2.64],
                     [2.2, 2.3, .35], 5, 8)
        m.sphere('Rose emblem centre', 'bud_plum', [0, 3, sign*3.04], [1.45, 1.65, .4], 5, 8)
        vine = bezier([0, 10, sign*2.03], [-4, 19, sign*1.75],
                      [4, 25, sign*1.8], [0, 34, sign*2.03], 20)
        m.tube('Botanical blade vine relief', 'leaf_green', vine, .23, 5)
        for side, y in [(-1, 15), (1, 23), (-1, 29)]:
            x = side*1.0
            xy = [[x, y-2], [side*4.8, y-1], [side*5.8, y+2.8],
                  [side*2.6, y+2.5], [x, y+1]]
            _leaf(m, 'Blade small rose leaf', xy, 1.94, sign, depth=.45)
        m.tube('Fairy guard growing vine', 'gold',
               [[-5, -13, sign*2.55], [-4, -6, sign*2.55], [0, -4.2, sign*2.55],
                [4, -6, sign*2.55], [5, -13, sign*2.55]], .26, 5)
    m.notes = ['Broad pink/rose steel blade with the canonical green foliage guard and dark rose botanical emblem.',
               'Leaf veins, vine relief, rose-bud pommel and two-sided green blade emblem are modeled solids.',
               'All new materials are opaque; native MM translucent GI routing remains available to the root fallback.']
    return _finish(m, -12)


def iron_knuckle_axe():
    m = _model('iron_knuckle_axe', "Iron Knuckle's Axe")
    m.material('shaft_blocks', [.38, .28, .17], 'wood', .05, .78)
    m.material('blue_jewel', [.10, .23, .44], None, .55, .23)
    # This outline is reconstructed from the checkout's actual 69-vtx inline
    # Iron Knuckle axe: two crescent blades, central gold spike, narrow shaft.
    # Original X is centered at 395 and -Z is the new upright Y, scaled 1/70.
    source_outline = [(1201, -2034), (2039, -2927), (2207, -3688),
                      (1910, -4677), (1219, -5346), (1016, -4507),
                      (395, -4153), (395, -3519), (1109, -2842)]
    right = np.array([[(x-395)/70, (-z-1571)/70] for x, z in source_outline])
    for side in (-1, 1):
        xy = right.copy()
        xy[:, 0] *= side
        if side < 0:
            xy = xy[::-1]
        _beveled_plate(m, 'Iron Knuckle crescent blade', xy, 'steel', 'blade_edge', 4.6)
        # Dark steel reinforcing ribs stop inside the polished cutting bevel.
        for sign in (-1, 1):
            m.tube('Axe forged strengthening rib', 'steel_shadow',
                   [[side*2.5, 31.5, sign*2.25], [side*9.5, 31.6, sign*2.30],
                    [side*16, 31.4, sign*2.25], [side*21.2, 30.9, sign*1.7]], .45, 6)
    m.polygon('Gold central head spike', 'gold', [(-4.7, 33), (4.7, 33),
              (4.2, 42), (0, 52.7), (-4.2, 42)], 6.6, 0, .85)
    for sign in (-1, 1):
        m.polygon('Head spike raised golden ridge', 'gold_edge',
                  [(-.65, 35), (.65, 35), (0, 50.5)], .28, sign*3.42, .09)
    m.lathe('Long patterned axe shaft', 'shaft_blocks', [(-47, 2.35),
              (-42, 2.35), (-22, 2.2), (0, 2.15), (19, 2.5), (34, 2.9)], 10)
    for y in (-43.5, -18.5, 7.5, 23.5, 33.5):
        _band(m, 'Axe shaft gold ferrule', 'gold', y, 3.05 if y < 20 else 4.2, .85)
        _band(m, 'Axe ferrule bright lip', 'gold_edge', y-.7,
              3.13 if y < 20 else 4.3, .25)
    _grip(m, -42, -23, 2.5, [.24, .145, .065], 'gold')
    m.lathe('Axe head square-sided gold socket', 'gold',
            [(20, 2.7), (25, 4.9), (34, 5.1), (40, 3.4)], 4)
    m.crystal('Axe faceted blue butt jewel', 'blue_jewel', [0, -50, 0], 5.4, 15, 4)
    _band(m, 'Axe butt jewel gold bezel', 'gold_edge', -46, 5.2, .6, 4)
    for sign in (-1, 1):
        for side in (-1, 1):
            m.sphere('Head socket forged rivet', 'gold_edge', [side*3.5, 30.3, sign*3.15],
                     [.8, .8, .48], 4, 8)
    m.notes = ['Original inline gIKAxeInlineDL identifies the double-crescent outline, gold central spike and jewel butt.',
               'Head outline is derived from those actual vertex positions, recentered at source x=395, mapped -Z to upright Y at 1/70.',
               'New broad steel bevels, gold socket/ferrules, wood/leather shaft detail and faceted blue butt jewel.',
               'Hammer upgrade equipment presentation only; actor/held axe and throw behavior remain unchanged.']
    return _finish(m, -11)


BUILDERS = {
    'kokiri_sword': kokiri_sword,
    'razor_sword': razor_sword,
    'gilded_sword': gilded_sword,
    'master_sword': master_sword,
    'true_master_sword': true_master_sword,
    'biggoron_sword': biggoron_sword,
    'great_fairy_sword': great_fairy_sword,
    'iron_knuckle_axe': iron_knuckle_axe,
}


def main():
    from build_completion import build, ROOT
    from pathlib import Path
    import subprocess
    import sys
    # The supplied forged recipes own these GI resources. Keep the remaining
    # legacy weapon builders for Razor, Biggoron and the Iron Knuckle axe.
    forged = {'kokiri_sword', 'mm_kokiri_sword', 'gilded_sword',
              'master_sword', 'true_master_sword', 'great_fairy_sword', 'four_sword'}
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('items', nargs='*')
    p.add_argument('--install', action='store_true')
    args = p.parse_args()
    names = args.items or list(BUILDERS) + ['mm_kokiri_sword', 'four_sword']
    unknown = set(names) - set(BUILDERS) - forged
    if unknown:
        p.error('Unknown sword candidate: ' + ', '.join(sorted(unknown)))
    chosen = [n for n in names if n in forged]
    if chosen:
        helper = Path(__file__).with_name('forged_swords') / 'rebuild.py'
        command = [sys.executable, str(helper), *chosen, '--output', str(ROOT)]
        if args.install:
            command.append('--install')
        subprocess.run(command, check=True)
    build({n: BUILDERS[n] for n in names if n not in forged}, ROOT, args.install)


if __name__ == '__main__':
    main()
