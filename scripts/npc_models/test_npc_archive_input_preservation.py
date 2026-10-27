"""Repair destinations must never alias protected input inodes."""
import contextlib
import io
import hashlib
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile

from native_assets import o2r
import recolor_saria_iris as iris
import refine_saria_ocarina as prop
import repair_npc_face_torso as face
import repair_twins_shoulders as twins
import repair_young_fado_shoulders as fado
import validate_npc_review_bundle as bundle_check


class InputPreservationTests(unittest.TestCase):
    def test_iris_audit_destinations_cannot_alias_inputs_or_archive_output(self):
        for target in ('saria-render-report.json', 'saria-render-exact-changed-pixel-masks.npz',
                       'saria-render-EyesClosed-mask.png'):
            for protected in ('native', 'parent', 'source', 'output'):
                for symlink in (False, True):
                    with self.subTest(target=target, protected=protected, symlink=symlink), tempfile.TemporaryDirectory() as tmp:
                        root = Path(tmp); audit = root/'audit'; audit.mkdir()
                        paths = {name: root/name for name in ('native', 'parent', 'source', 'output')}
                        for path in paths.values(): path.write_bytes(b'protected bytes')
                        metadata = dict(variant='saria-render', namespace='objects/object_sa/NativeNpcV1',
                            source_sha256='902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff',
                            native_sha256=hashlib.sha256(paths['native'].read_bytes()).hexdigest())
                        with zipfile.ZipFile(paths['parent'], 'w') as archive:
                            archive.writestr('saria_NativeNpcModelInfo.json', json.dumps(metadata))
                        before = {name: path.read_bytes() for name, path in paths.items()}
                        if symlink: (audit/target).symlink_to(paths[protected])
                        else: os.link(paths[protected], audit/target)
                        # Native palette lookup is unrelated to destination ownership;
                        # the real archive writer and audit writes remain exercised.
                        with mock.patch.object(iris, 'native_blue_reference', return_value=(.65, {})):
                            with self.assertRaisesRegex(ValueError, 'separate'):
                                iris.recolor(paths['native'], paths['parent'], paths['output'], paths['source'], audit)
                        self.assertEqual({name: path.read_bytes() for name, path in paths.items()}, before)

    def test_twins_previews_cannot_alias_source_archives(self):
        from PIL import Image
        for target in ('previews/parent-static0.png', 'previews/candidate-static5.png',
                       'Twins_Shoulder_Sampler_Comparison.png'):
            for protected in ('native', 'parent', 'candidate'):
                for symlink in (False, True):
                    with self.subTest(target=target, protected=protected, symlink=symlink), tempfile.TemporaryDirectory() as tmp:
                        root = Path(tmp); destination = root/'audit'; (destination/'previews').mkdir(parents=True)
                        paths = {name: root/name for name in ('native', 'parent', 'candidate')}
                        for path in paths.values(): path.write_bytes(b'protected bytes')
                        if symlink: (destination/target).symlink_to(paths[protected])
                        else: os.link(paths[protected], destination/target)
                        def preview(task):
                            Image.new('RGB', (8, 8)).save(destination/'previews'/(task[-1]['name']+'.png'))
                        with mock.patch('preview_native_npc_models.preview', side_effect=preview):
                            with self.assertRaisesRegex(ValueError, 'separate'):
                                twins.preview_comparison(paths['native'], paths['parent'], paths['candidate'], destination, 8)
                        for path in paths.values(): self.assertEqual(path.read_bytes(), b'protected bytes')

    def test_bundle_readback_report_cannot_overwrite_native_archive(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); bundle = root/'bundle'; paths = []
            for i in range(4): paths.append(bundle/'Common'/f'model{i}.o2r')
            for style in ('Anime','Traditional'):
                paths.append(bundle/'Choose-One-Saria'/style/'saria.o2r')
            native, retained = root/'native.o2r', root/'retained.o2r'
            for i, path in enumerate([*paths,native,retained]):
                path.parent.mkdir(parents=True, exist_ok=True)
                with zipfile.ZipFile(path,'w') as archive:
                    archive.writestr(f'private-{i}.json', '{}')
            before = native.read_bytes()
            argv = ['audit','--repo',str(Path(__file__).resolve().parents[2]),
                    '--bundle',str(bundle),'--native',str(native),
                    '--retained-adult-fado',str(retained),'--report',str(native)]
            with mock.patch('sys.argv', argv), contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaisesRegex(ValueError,'separate'):
                    bundle_check.main()
            self.assertEqual(native.read_bytes(),before)

    def test_refinement_refuses_hardlinked_parent_before_writing(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); parent = root/'parent.o2r'; output = root/'output.o2r'
            ns = 'objects/object_sa/NativeNpcV1'
            dl = o2r.DLBuilder(); dl.settimg_hash(0, 2, 32, 'objects/object_sa/gSariaFairyOcarinaTex'); dl.end()
            with zipfile.ZipFile(parent, 'w') as archive:
                archive.writestr('saria_NativeNpcModelInfo.json', json.dumps(dict(
                    variant='saria-anime', namespace=ns, geometry_repairs={})))
                for suffix in ('', 'Grip'):
                    archive.writestr('alt/'+ns+'/Props/Ocarina'+suffix, dl.to_resource())
            before = parent.read_bytes(); os.link(parent, output)
            with self.assertRaisesRegex(ValueError, 'separate'):
                prop.refine(parent, output)
            self.assertEqual(parent.read_bytes(), before)

    def test_each_archive_repair_refuses_hardlinked_inputs_before_parsing(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            inputs = {name: root/name for name in ('native', 'parent', 'source', 'art')}
            for path in inputs.values(): path.write_bytes(b'protected input')
            calls = [
                ('face-native', 'native', lambda out: face.build(inputs['native'], inputs['parent'], 'malon', out)),
                ('face-parent', 'parent', lambda out: face.build(inputs['native'], inputs['parent'], 'malon', out)),
                ('face-source', 'source', lambda out: face.build(inputs['native'], inputs['parent'], 'saria-anime', out,
                                                                inputs['source'], inputs['art'])),
                ('face-art', 'art', lambda out: face.build(inputs['native'], inputs['parent'], 'saria-anime', out,
                                                          inputs['source'], inputs['art'])),
                ('twins-native', 'native', lambda out: twins.repair(inputs['native'], inputs['parent'], out)),
                ('twins-parent', 'parent', lambda out: twins.repair(inputs['native'], inputs['parent'], out)),
                ('iris-native', 'native', lambda out: iris.recolor(inputs['native'], inputs['parent'], out, inputs['source'])),
                ('iris-parent', 'parent', lambda out: iris.recolor(inputs['native'], inputs['parent'], out, inputs['source'])),
                ('iris-source', 'source', lambda out: iris.recolor(inputs['native'], inputs['parent'], out, inputs['source'])),
                *[(f'fado-{name}', name, lambda out: fado.build(inputs['native'], inputs['parent'],
                    inputs['source'], inputs['art'], out)) for name in inputs],
            ]
            for label, name, invoke in calls:
                with self.subTest(repair=label):
                    output = root/(label+'.o2r'); os.link(inputs[name], output)
                    with self.assertRaisesRegex(ValueError, 'separate'):
                        invoke(output)
                    self.assertEqual(inputs[name].read_bytes(), b'protected input')

    def test_face_report_refuses_input_and_output_aliases_before_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = {name: root/name for name in ('native', 'parent', 'output', 'source', 'art')}
            for path in paths.values(): path.write_bytes(b'protected bytes')
            for name, path in paths.items():
                for hardlink in (False, True):
                    with self.subTest(target=name, hardlink=hardlink):
                        report = root/('report-'+name) if hardlink else path
                        if hardlink: os.link(path, report)
                        argv = ['face', str(paths['native']), str(paths['parent']), 'saria-anime',
                                str(paths['output']), '--eye-source', str(paths['source']),
                                '--eye-art', str(paths['art']), '--report', str(report)]
                        with mock.patch('sys.argv', argv), mock.patch.object(face, 'build', return_value={}) as build:
                            with contextlib.redirect_stdout(io.StringIO()):
                                with self.assertRaisesRegex(ValueError, 'separate'):
                                    face.main()
                            build.assert_not_called()
                        self.assertEqual(path.read_bytes(), b'protected bytes')

    def test_fado_report_refuses_input_and_output_aliases_before_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = {name: root/name for name in ('native', 'parent', 'source', 'mesh', 'output')}
            for path in paths.values(): path.write_bytes(b'protected bytes')
            for name, path in paths.items():
                for hardlink in (False, True):
                    with self.subTest(target=name, hardlink=hardlink):
                        report = root/('report-'+name) if hardlink else path
                        if hardlink: os.link(path, report)
                        argv = ['fado', *[str(paths[name]) for name in ('native','parent','source','mesh','output')],
                                '--report', str(report)]
                        with mock.patch('sys.argv', argv), mock.patch.object(fado, 'build', return_value={}) as build:
                            with contextlib.redirect_stdout(io.StringIO()):
                                with self.assertRaisesRegex(ValueError, 'separate'):
                                    fado.main()
                            build.assert_not_called()
                        self.assertEqual(path.read_bytes(), b'protected bytes')


if __name__ == '__main__':
    unittest.main()
