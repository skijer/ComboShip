"""Cojiro and Mario Mask GI candidates in the accepted serialized mesh pipeline.

These are GI-only objects. Native actors, player meshes, and held resources are
separate, and the normal owner/mod selection must run before these fallbacks.
"""
import argparse
from pathlib import Path

from meshkit import Model, rotation


def ellipsoid(model, name, material, center, radii, tilt=0, rows=10, sides=20):
    start = len(model.parts)
    model.sphere(name, material, [0, 0, 0], radii, rows, sides)
    model.transform(rotation('z', tilt), offset=center, start=start)


def cojiro():
    m = Model('cojiro', 'Cojiro', 'objects/nei_gi_redesign/cojiro/gi_dl', 1., .65)
    for name, color in {
        'blue': [.12, .43, .83],
        'light_blue': [.32, .65, .98],
        'breast': [.53, .78, 1.],
        'wing_blue': [.09, .32, .68],
        'feather_edge': [.19, .50, .89],
        'tail': [.07, .23, .55],
        'red': [.88, .095, .15],
        'comb_light': [.98, .18, .22],
        'yellow': [1., .66, .095],
        'feet': [.89, .42, .065],
        'white': [.99, .98, .92],
        'black': [.015, .028, .065],
    }.items():
        m.material(name, color, rough=.7 if name not in ('black', 'white') else .35)

    # Plump toy-like silhouette and a raised, rounded breast distinguish the
    # blue trade cucco without borrowing the native actor or an HD pack's mesh.
    ellipsoid(m, 'Round blue cucco body', 'blue', [0, -5, -1], [22, 24, 18], rows=12, sides=24)
    ellipsoid(m, 'Soft sky blue breast', 'breast', [0, -5, 13.2], [16, 19, 5.4], rows=12, sides=24)
    for side in (-1, 1):
        ellipsoid(m, 'Rounded folded wing', 'wing_blue', [side*19, -5, 1], [7.7, 15.8, 10],
                  tilt=side*12, rows=10, sides=20)
        for index in range(3):
            ellipsoid(m, 'Layered wing feather', 'feather_edge',
                      [side*(21.2-index*.8), -11+index*4.6, 7.9-index*.6],
                      [3.8, 8.2-index*.7, 3.1], tilt=side*(20+index*4), rows=8, sides=16)

    # A small fan is readable from the rear and remains within the GI cylinder.
    for index, (x, y, tilt) in enumerate([(-10, 6, -27), (-5, 13, -14),
                                         (0, 16, 0), (5, 13, 14), (10, 6, 27)]):
        ellipsoid(m, 'Upright blue tail feather', 'tail' if index % 2 else 'wing_blue',
                  [x, y, -17.5], [5.4, 15, 4.4], tilt=tilt, rows=10, sides=16)
        ellipsoid(m, 'Tail feather ridge', 'feather_edge', [x, y+2, -21.1],
                  [1.1, 10, .8], tilt=tilt, rows=6, sides=12)

    ellipsoid(m, 'Large friendly blue head', 'light_blue', [0, 20, 8], [17, 17, 15],
              rows=14, sides=28)
    for side in (-1, 1):
        ellipsoid(m, 'Soft cheek feather', 'breast', [side*10.3, 14, 18.2], [5.8, 6, 3.6],
                  tilt=side*12, rows=8, sides=16)
        ellipsoid(m, 'Wide white cucco eye', 'white', [side*7.2, 24, 20.3], [5.1, 6.5, 2.4],
                  tilt=side*9, rows=10, sides=20)
        ellipsoid(m, 'Round attentive pupil', 'black', [side*6.3, 24.2, 22.5], [2.45, 3.8, 1.2],
                  rows=10, sides=16)
        ellipsoid(m, 'Eye catchlight', 'white', [side*6.3-.7, 25.7, 23.4], [.8, 1.1, .45],
                  rows=6, sides=12)
    m.tube('Golden upper beak', 'yellow', [[0, 16.7, 20.5], [0, 16, 27.5], [0, 14.8, 31]],
           [5.2, 3.2, .2], sides=10)
    m.tube('Golden lower beak', 'feet', [[0, 13, 21.5], [0, 13.5, 26.5], [0, 14, 29.5]],
           [3.8, 2.2, .2], sides=10)
    for side in (-1, 1):
        ellipsoid(m, 'Small red wattle', 'red', [side*2.6, 8.6, 22.8], [3.3, 6.2, 2.6],
                  tilt=side*10, rows=8, sides=16)
    for index, (y, z, height) in enumerate([(37.2, 13, 6.5), (40.3, 7.1, 8), (38.3, 1, 6.8)]):
        ellipsoid(m, 'Rounded red comb lobe', 'comb_light' if index == 1 else 'red',
                  [0, y, z], [3.7, height, 4.8], rows=10, sides=16)

    for side in (-1, 1):
        x = side*9
        m.tube('Short cucco shin', 'feet', [[x, -24, 3], [x, -33.5, 4], [x, -36.5, 7]],
               [2.4, 1.8, 1.7], sides=10)
        for spread, length in [(-4.7, 9.2), (0, 12.3), (4.7, 9.2)]:
            m.tube('Rounded golden toe', 'yellow',
                   [[x, -36.8, 6], [x+spread*.7, -37.6, 6+length*.65],
                    [x+spread, -37.3, 6+length]], [1.7, 1.45, .65], sides=8)
    m.notes = ['GI-only cute blue Cojiro with rounded plumage, wide eyes, red comb and golden feet.',
               'Volumetric front, sides and tail; native actor and user model paths stay separate.',
               'Authored front faces +Z. The shared GI renderer supplies its normal Y-axis spin.']
    return m


def legacy_mario_mask():
    """Earlier sculpted-mask recipe, retained for historical inspection only."""
    m = Model('mario_mask', 'Mario Mask', 'objects/nei_gi_redesign/mario_mask/gi_dl', 1., .65)
    for name, color in {
        'skin': [1., .65, .39],
        'cheek': [1., .73, .48],
        'inner_ear': [.80, .36, .21],
        'cap': [.83, .025, .045],
        'cap_light': [.97, .055, .065],
        'cap_band': [.62, .015, .025],
        'hair': [.15, .045, .018],
        'mustache': [.045, .016, .007],
        'white': [1., .98, .89],
        'blue': [.02, .33, .88],
        'pupil': [.008, .013, .025],
        'backing': [.12, .065, .035],
    }.items():
        m.material(name, color, rough=.8 if name in ('hair', 'mustache', 'backing') else .5)

    # The shallow padded back and closed edge rim make this a wearable mask,
    # readable during the reverse half of a pickup turn, not a one-sided card.
    ellipsoid(m, 'Dark padded mask backing', 'backing', [0, -1, -1.4], [25.5, 29, 5.8], rows=12, sides=28)
    ellipsoid(m, 'Rounded Mario mask face', 'skin', [0, -1, 4], [24, 28, 10], rows=14, sides=28)
    for side in (-1, 1):
        ellipsoid(m, 'Brown side hair', 'hair', [side*21.8, 2.8, 1.5], [5.5, 17.5, 7],
                  tilt=side*7, rows=10, sides=20)
        ellipsoid(m, 'Rounded ear', 'skin', [side*25.5, -1, 5], [5.7, 8, 4.8], rows=10, sides=20)
        ellipsoid(m, 'Inner ear fold', 'inner_ear', [side*26.4, -1.1, 8.7], [2.7, 4.8, 1.4], rows=8, sides=16)
        ellipsoid(m, 'Raised smiling cheek', 'cheek', [side*15.2, -9.4, 11.8], [8, 9, 5.7], rows=10, sides=20)
        ellipsoid(m, 'Mario eye white', 'white', [side*8.3, 7.2, 13], [5.9, 8.3, 2.4],
                  tilt=side*6, rows=12, sides=20)
        ellipsoid(m, 'Bright blue iris', 'blue', [side*7.7, 7.4, 15.1], [2.65, 4.8, 1.2], rows=10, sides=18)
        ellipsoid(m, 'Dark eye pupil', 'pupil', [side*7.6, 7.5, 16.15], [1.4, 3, .7], rows=10, sides=16)
        ellipsoid(m, 'Small eye shine', 'white', [side*7.6-.7, 9.3, 16.65], [.6, .9, .3], rows=6, sides=12)
        m.tube('Thick arched eyebrow', 'hair',
               [[side*3.3, 15, 12.6], [side*7.6, 17.6, 13.8], [side*12.3, 16, 11.9]],
               [1.8, 2.3, 1.6], sides=10)

    # Six rounded mustache lobes keep the silhouette recognizable at icon size.
    for side in (-1, 1):
        for x, y, rx, ry in [(5.5, -9, 5.5, 4.8), (11.5, -8.4, 4.7, 4.4), (16.3, -6.6, 3.9, 4)]:
            ellipsoid(m, 'Sculpted mustache lobe', 'mustache', [side*x, y, 16.9], [rx, ry, 3.5],
                      tilt=side*24, rows=10, sides=16)
    m.tube('Subtle smile crease', 'inner_ear', [[-5, -17.2, 13.4], [0, -18, 14.1], [5, -17.2, 13.4]],
           [.6, .7, .6], sides=8)
    ellipsoid(m, 'Round projecting Mario nose', 'cheek', [0, -1.3, 18.9], [8.4, 7.2, 7.6], rows=14, sides=24)

    ellipsoid(m, 'Red cloth cap crown', 'cap', [0, 24.4, -.3], [27.5, 15.5, 13.5], rows=12, sides=28)
    ellipsoid(m, 'Cap forehead band', 'cap_band', [0, 17.4, 6.5], [25, 4.8, 10], rows=10, sides=24)
    ellipsoid(m, 'Curved red cap brim', 'cap_light', [0, 16, 14.2], [26.8, 3.25, 15.4], rows=10, sides=32)
    ellipsoid(m, 'Cream cap emblem', 'white', [0, 27.3, 12.35], [8.8, 8.1, 1.2], rows=12, sides=24)
    # An actual raised M, with closed edges, avoids billboards or baked artwork.
    m.polygon('Raised red M emblem', 'cap_light',
              [(-5.6, 22.8), (-5.6, 31.8), (-2.6, 31.8), (0, 27.5),
               (2.6, 31.8), (5.6, 31.8), (5.6, 22.8), (2.8, 22.8),
               (2.8, 27.2), (0, 23.6), (-2.8, 27.2), (-2.8, 22.8)],
              depth=.8, z=13.65, bevel=.15, closed_sides=True)
    # Two anchored leather ties identify the reverse as the inside of a mask.
    for side in (-1, 1):
        m.tube('Rear mask strap', 'backing',
               [[side*25, 3.5, 0], [side*26, 5, -7], [side*18, 7, -13], [0, 8, -15]],
               [1.8, 1.7, 1.6, 1.5], sides=8)
    m.notes = ['GI-only sculpted Mario face mask, red M cap, blue eyes and rounded mustache.',
               'Fully volumetric front, closed sides, shallow padded reverse and leather straps.',
               'No native Mario actor/player mesh or held-model resource is replaced.',
               'Authored front faces +Z. The shared GI renderer supplies its normal Y-axis spin.']
    return m


BUILDERS = {'cojiro': cojiro}
RETAINED_ITEMS = ('mario_mask',)


def main():
    from build_completion import build
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('items', nargs='*')
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    names = args.items or list(BUILDERS) + list(RETAINED_ITEMS)
    for name in names:
        if name not in BUILDERS and name not in RETAINED_ITEMS:
            parser.error('Unknown GI candidate: ' + name)
    import shutil
    root = Path(__file__).resolve().parents[1]
    for name in names:
        if name == 'mario_mask':
            from mario_sm64_poc3 import rebuild
            rebuild(args.install)
            continue
        build({name: BUILDERS[name]}, root, args.install and name != 'cojiro')
        if name == 'cojiro':
            relative = Path('objects/nei_gi_redesign/cojiro')
            shutil.copytree(root / 'RESOURCES' / relative, root / 'OPTIONAL_ASSETS' / relative, dirs_exist_ok=True)


if __name__ == '__main__':
    main()
