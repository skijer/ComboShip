"""Tests for source material selection and native rig compatibility."""
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from types import SimpleNamespace

from PIL import Image

try:
    import native_npc_models as compiler
except ModuleNotFoundError:
    compiler = None


def fixture_glb(path):
    """A normal map precedes the diffuse image, as in the supplied Tripo files."""
    binary = bytearray()
    views = []
    for color in [(0, 0, 255), (235, 130, 60)]:
        stream = io.BytesIO()
        Image.new("RGB", (2, 2), color).save(stream, format="PNG")
        views.append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(stream.getvalue())})
        binary.extend(stream.getvalue())
    document = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": []}],
                "nodes": [], "meshes": [], "skins": [], "accessors": [], "bufferViews": views,
                "buffers": [{"byteLength": len(binary)}],
                "images": [{"bufferView": 0, "mimeType": "image/png"},
                           {"bufferView": 1, "mimeType": "image/png"}],
                "textures": [{"source": 0}, {"source": 1}],
                "materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 1}}}]}
    raw = json.dumps(document).encode()
    raw += b" " * (-len(raw) % 4)
    binary.extend(b"\0" * (-len(binary) % 4))
    path.write_bytes(struct.pack("<III", 0x46546C67, 2, 28 + len(raw) + len(binary)) +
                     struct.pack("<I4s", len(raw), b"JSON") + raw +
                     struct.pack("<I4s", len(binary), b"BIN\0") + binary)


def proportion_fixture():
    """A short source torso against a wide, long native torso."""
    import numpy as np
    from native_assets import skeleton
    positions = {"Hips": [0, .5, 0], "Neck": [0, .75, 0], "Head": [0, .85, 0]}
    for side, sign in [("Left", 1), ("Right", -1)]:
        for name, p in {"UpLeg": [.08, .48, 0], "Leg": [.08, .28, 0], "Foot": [.08, .08, 0],
                        "Arm": [.14, .7, 0], "ForeArm": [.3, .7, 0], "Hand": [.45, .7, 0]}.items():
            positions[side+name] = [p[0]*sign, p[1], p[2]]
    positions["LeftShoulder"] = [.05, .7, 0]
    names = list(positions)
    world = {}
    for i, name in enumerate(names):
        world[i] = np.eye(4)
        world[i][:3, 3] = positions[name]
    meshes = []
    for name, points in [("Hips", [[-.08, .52, 0], [.08, .52, 0], [0, .72, .03]]),
                         ("Head", [[-.04, .83, 0], [.04, .83, 0], [0, .98, .02]]),
                         ("LeftFoot", [[.06, 0, 0], [.1, 0, 0], [.08, .08, .02]]),
                         ("RightFoot", [[-.06, 0, 0], [-.1, 0, 0], [-.08, .08, .02]]),
                         ("LeftShoulder", [[.17, .7, 0], [.19, .7, 0], [.18, .7, .02]])]:
        meshes.append(dict(positions=np.array(points), normals=np.tile([0., 0., 1.], (3, 1)),
                           uv=np.zeros((3, 2)), joints=np.full((3, 1), names.index(name)),
                           weights=np.ones((3, 1)), faces=np.array([[0, 1, 2]])))
    source = SimpleNamespace(names=names, joints=list(range(len(names))), world=world,
                             inverse=[np.linalg.inv(world[i]) for i in range(len(names))],
                             meshes=meshes, image=Image.new("RGBA", (2, 2), "white"))
    # Child/sibling order and DL slots match the ordinary 15-limb girl layout.
    layout = [(0, 2000, 0, 1, 255), (320, -100, 0, 2, 4), (0, -700, 0, 3, 255),
              (0, -700, 0, 255, 255), (-320, -100, 0, 5, 7), (0, -700, 0, 6, 255),
              (0, -700, 0, 255, 255), (0, 0, 0, 8, 255), (850, 1250, 0, 9, 11),
              (650, 0, 0, 10, 255), (650, 0, 0, 255, 255), (-850, 1250, 0, 12, 14),
              (-650, 0, 0, 13, 255), (-650, 0, 0, 255, 255), (0, 1800, 0, 255, 255)]
    limbs = [skeleton.Limb(i, *row, 1) for i, row in enumerate(layout)]
    frame = np.zeros((16, 3), dtype=int)
    frame[0] = [0, 2000, 0]
    rig = SimpleNamespace(name="twins", profile=compiler.PROFILES["twins"], limbs=limbs,
                          skel=skeleton.Skeleton(limbs, 15, 15), frames=np.array([frame]))
    rig.pose = lambda value: skeleton._pose(rig.skel, np.asarray(value).ravel().tolist())
    rig.pose(frame)
    return source, rig


class ExportTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(compiler, "The native NPC archive compiler is missing")

    def test_uses_material_diffuse_instead_of_first_image(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "fixture.glb"
            fixture_glb(path)
            source = compiler.SourceModel(path)
            self.assertEqual(source.image.getpixel((0, 0)), (235, 130, 60, 255))

    def test_kokiri_girl_scope_covers_every_native_placement(self):
        self.assertEqual(compiler.PROFILES["twins"]["native_actor_types"], [1, 5, 6, 9, 10])

    def test_saria_variants_share_the_same_actor_contract(self):
        a = compiler.VARIANTS["saria-anime"]
        b = compiler.VARIANTS["saria-render"]
        self.assertEqual(a["profile"], "saria")
        self.assertEqual(a["profile"], b["profile"])
        self.assertNotEqual(a["archive"], b["archive"])

    def test_rotation_handles_opposite_vectors_without_nan(self):
        import numpy as np
        rotation = compiler.align([0, 1, 0], [0, -1, 0])
        np.testing.assert_allclose(rotation @ [0, 1, 0], [0, -1, 0], atol=1e-8)
        np.testing.assert_allclose(rotation.T @ rotation, np.eye(3), atol=1e-8)

    def test_animation_sampler_rejects_out_of_bounds_native_channels(self):
        raw = bytearray(compiler.asset._header(0x4F414E4D))
        raw += struct.pack("<IhIhIHHHH", 0, 1, 1, 0, 1, 0, 20, 0, 1)
        with self.assertRaises(ValueError):
            compiler.animation_frames(bytes(raw))

    def test_preview_rig_reads_private_limb_offsets(self):
        import numpy as np
        from edit_native_npc_pose import encode_animation
        compiler.PROFILES["fixture"] = dict(object="object_fixture", skeleton="Skel", animation="Anim")
        ns = "objects/object_fixture/NativeNpcV1"
        limb_path = ns+"/Rig/Limb00"
        skel = compiler.asset.build_skeleton([limb_path], 1,
                                             compiler.asset.SKEL_FLEX, 1)
        animation = encode_animation(np.zeros((1, 2, 3), dtype=int))
        class Fixture:
            def read(self, path):
                return {ns+"/Skel": skel,
                        limb_path: compiler.asset.build_limb((11, 22, 33), 255, 255, ns+"/Body/Limb00", "", 1),
                        "objects/object_fixture/Anim": animation}[path]
        try:
            rig = compiler.NativeRig(Fixture(), "fixture", private=True)
            self.assertEqual((rig.limbs[0].x, rig.limbs[0].y, rig.limbs[0].z), (11, 22, 33))
        finally:
            del compiler.PROFILES["fixture"]

    def test_pose_editor_round_trip_preserves_root_and_other_joints(self):
        import numpy as np
        import zipfile
        import hashlib
        from edit_native_npc_pose import edit, encode_animation
        frames = np.array([[[10, 20, 30], [100, 200, 300]], [[10, 20, 30], [400, 500, 600]]])
        np.testing.assert_array_equal(compiler.animation_frames(encode_animation(frames)), frames)
        with tempfile.TemporaryDirectory() as temp:
            base, native, output = [Path(temp)/name for name in ["model.o2r", "native.o2r", "fit.o2r"]]
            path = "objects/object_os_anime/testAnim"
            with zipfile.ZipFile(native, "w") as z:
                z.writestr(path, encode_animation(frames))
            with zipfile.ZipFile(base, "w") as z:
                z.writestr("test_NativeNpcModelInfo.json", json.dumps(dict(namespace="objects/test/NativeNpcV1", target_limbs=1, variant="test")))
                z.writestr("alt/objects/test/NativeNpcV1/Body/Limb00", b"untouched")
            recipe = dict(schema_version=1, parent_sha256=hashlib.sha256(base.read_bytes()).hexdigest(),
                          clips=[dict(native_animation=path, rotations=[dict(joint=1, mode="add", binang=[1000, 0, 0])])])
            edit(base, native, recipe, output)
            with zipfile.ZipFile(output) as z:
                actual = compiler.animation_frames(z.read("alt/objects/test/NativeNpcV1/PoseAnimations/"+path))
                np.testing.assert_array_equal(actual[:, 0], frames[:, 0])
                np.testing.assert_array_equal(actual[:, 1, 0], frames[:, 1, 0]+1000)
                self.assertEqual(z.read("alt/objects/test/NativeNpcV1/Body/Limb00"), b"untouched")
            recipe["clips"][0]["rotations"][0]["joint"] = 0
            with self.assertRaisesRegex(ValueError, "root motion"):
                edit(base, native, recipe, output)

    def test_split_surface_join_keeps_one_native_owner(self):
        import numpy as np
        self.assertTrue(hasattr(compiler, "join_split_surfaces"), "Split Tripo surface joins need native seam fitting")
        model = dict(source_positions=np.array([[0., 0., 0.], [.001, 0., 0.]]),
                     positions=np.array([[0., 0., 0.], [100., 0., 0.]]),
                     bones=np.array([0, 1]), weights=np.array([[1., 0.], [0., 1.]]),
                     mesh_ids=np.array([0, 1]), boundary=np.array([True, True]))
        compiler.join_split_surfaces(model, 1000, -1)
        np.testing.assert_allclose(model["positions"][0], model["positions"][1])
        self.assertEqual(model["bones"][0], model["bones"][1])

    def test_fitting_preserves_source_torso_and_head_proportions(self):
        import numpy as np
        source, rig = proportion_fixture()
        original_root = rig.frames[:, 0].copy()
        topology = [(l.child, l.sibling, l.dlist) for l in rig.limbs]
        model = compiler.fit_model(source, rig)
        # The same scale must fit horizontal torso, vertical torso and head.
        expected = model["source_positions"][:6]*model["scale"]
        expected += [0, 2000-source.world[0][1, 3]*model["scale"], 0]
        np.testing.assert_allclose(model["positions"][:6], expected, atol=2.)
        np.testing.assert_array_equal(rig.frames[:, 0], original_root)
        self.assertEqual([(l.child, l.sibling, l.dlist) for l in rig.limbs], topology)

    def test_outer_shoulder_surface_follows_lowered_upper_arm(self):
        import numpy as np
        source, rig = proportion_fixture()
        for index in [9, 10]:
            rig.limbs[index].x, rig.limbs[index].y = 0, -650
        rig.pose(rig.frames[0])
        model = compiler.fit_model(source, rig)
        shoulder = rig.limbs[8].T
        expected = np.array([[0, -.03, 0], [0, -.05, 0], [0, -.04, .02]])*model["scale"]+shoulder
        np.testing.assert_allclose(model["positions"][-3:], expected, atol=2.)

    def test_hand_frame_matches_native_fingers_and_thumb_without_reflection(self):
        import numpy as np
        self.assertTrue(hasattr(compiler, "hand_frame"), "Hand fitting must constrain palm roll as well as finger direction")
        # Native fingers extend along +X and the thumb lies towards -Y.
        # This source has the same palm rolled sideways about its fingers.
        frame = compiler.hand_frame(np.array([0., 0., 1.]), np.array([0., 1., 1.]))
        np.testing.assert_allclose(frame@np.array([0., 0., 1.]), [1, 0, 0], atol=1e-9)
        thumb = frame@np.array([0., 1., 1.])
        self.assertLess(thumb[1], 0)
        self.assertAlmostEqual(thumb[2], 0)
        self.assertAlmostEqual(np.linalg.det(frame), 1.)

    def test_arm_ik_keeps_bone_lengths_and_clamps_only_unreachable_targets(self):
        import numpy as np
        from native_npc_pose_fit import elbow_point
        shoulder = np.zeros(3)
        elbow = np.array([1., 1., 0.]); wrist = np.array([2., 0., 0.])
        for target in (np.array([1., 0., 1.]), np.array([20., 0., 0.])):
            new_elbow, new_wrist = elbow_point(shoulder, elbow, wrist, target)
            self.assertAlmostEqual(np.linalg.norm(new_elbow-shoulder), np.linalg.norm(elbow-shoulder))
            self.assertAlmostEqual(np.linalg.norm(new_wrist-new_elbow), np.linalg.norm(wrist-elbow))
            if np.linalg.norm(target) < 2*np.sqrt(2):
                np.testing.assert_allclose(new_wrist, target)
            else:
                self.assertLess(np.linalg.norm(new_wrist), 2*np.sqrt(2))

    def test_eye_shading_seam_excludes_open_mesh_edges(self):
        import numpy as np
        import native_npc_face as face
        self.assertTrue(hasattr(face, "shared_patch_vertices"), "UV splits must not block eyelid shading repair")
        p = np.array([[0., 0., 0.], [1., 0., 0.], [0., 1., 0.],
                      [1., 0., 0.], [2., 0., 0.], [2., 1., 0.]])
        faces = np.array([[0, 1, 2], [3, 4, 5]])
        np.testing.assert_array_equal(face.shared_patch_vertices(p, faces, faces[:1]), [1])

    def test_authored_blink_preserves_pixels_outside_eyes(self):
        import numpy as np
        import native_npc_face as face
        base = np.random.default_rng(6).integers(0, 256, (64, 64, 4), dtype=np.uint8)
        authored = np.zeros_like(base)
        bounds = face.face_bounds("twins")
        result = face.compose_eye_state(base, authored, "twins", bounds)
        mask = face.eye_mask("twins", bounds, size=64)
        np.testing.assert_array_equal(result[~mask], base[~mask])
        self.assertTrue(np.any(result[mask] != base[mask]))

    def test_face_uv_split_normals_share_smooth_skin_shading(self):
        import numpy as np
        self.assertTrue(hasattr(compiler, "weld_face_normals"))
        model = dict(source_positions=np.array([[.03, .85, .05], [.03, .85, .05], [0, .6, 0]]),
                     positions=np.array([[100., 300., 80.], [100., 300., 80.], [0., 0., 0.]]),
                     normals=np.array([[.8, 0., .6], [-.8, 0., .6], [0., 1., 0.]]),
                     bones=np.array([14, 14, 7]))
        original = model["positions"].copy()
        compiler.weld_face_normals(model, 14, .75, np.tile([220, 170, 110], (3, 1)))
        np.testing.assert_allclose(model["normals"][:2], [[0, 0, 1], [0, 0, 1]])
        np.testing.assert_array_equal(model["positions"], original)
        np.testing.assert_array_equal(model["normals"][2], [0, 1, 0])

    def test_closed_lid_has_one_surface_instead_of_overlapping_eye_shells(self):
        import numpy as np
        import native_npc_face as face
        self.assertTrue(hasattr(face, "make_lid_surface"))
        p = np.array([[0., 0., .1], [1., 0., .1], [1., 1., .1], [0., 1., .1],
                      [0., 0., .2], [1., 0., .2], [1., 1., .2], [0., 1., .2]])
        fs = np.array([[0, 1, 2], [0, 2, 3], [4, 5, 6], [4, 6, 7]])
        model = dict(source_positions=p, positions=p.copy(), normals=np.tile([0., 0., 1.], (8, 1)),
                     faces=fs, head_transform=np.column_stack([np.eye(3), np.zeros(3)]))
        original = model["positions"].copy()
        out = face.make_lid_surface(model, fs, np.array([.1, 0, 0, 0, 0, 0]), (0., 0.), [0, 0, 1, 1], 14)
        self.assertEqual(len(out["faces"]), 2)
        self.assertEqual(len(out["positions"]), 4)
        np.testing.assert_allclose(out["positions"][:, 2], .1)
        np.testing.assert_array_equal(model["positions"], original)


if __name__ == "__main__":
    unittest.main()
