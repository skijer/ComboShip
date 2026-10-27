// Run the actual Prelude manifest validator from an independently supplied editor bundle.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const source = fs.readFileSync(process.argv[2], 'utf8');
const input = JSON.parse(fs.readFileSync(0, 'utf8'));
const manifest = input.manifest;
const start = source.indexOf('var IHt,RZ,GHe=');
const end = source.indexOf(';var Bx=', start);
assert(start >= 0 && end > start, 'Unsupported Prelude bundle: validator anchors changed');
const context = vm.createContext({ TextEncoder, TextDecoder, vt: init => init,
  xi: 'alt/', fS: 'prelude/project/shadowed/', RO: async entries => entries });
vm.runInContext(source.slice(start, end + 1) + ';kZ();globalThis.parseManifest=YHe;', context);
const parse = data => context.parseManifest({
  file: path => path === 'prelude/manifest.json'
    ? new TextEncoder().encode(JSON.stringify(data)) : null,
});
const accepted = parse(manifest);
assert(accepted?.project, 'Actual Prelude parser rejected the editable project');
assert.equal(JSON.stringify(accepted.project.layers), JSON.stringify(manifest.project.layers));
const invalid = JSON.parse(JSON.stringify(manifest));
delete invalid.project.previewedAgainst;
assert.equal(parse(invalid)?.project, undefined, 'Negative control did not reject incomplete project');
const restoreStart = source.indexOf('async function Umn(');
const restoreEnd = source.indexOf('async function fne(', restoreStart);
assert(restoreStart >= 0 && restoreEnd > restoreStart, 'Prelude restore anchors changed');
vm.runInContext(source.slice(restoreStart, restoreEnd) + ';globalThis.restoreLayers=Umn;', context);
const files = Object.fromEntries(Object.entries(input.files).map(([p,b]) => [p, Buffer.from(b,'base64')]));
const bare = p => p.startsWith('alt/') ? p.slice(4) : p;
const canonicalFiles = Object.fromEntries(Object.entries(files).map(([p,b]) => [bare(p), b]));
(async () => {
  for (const layout of [files, canonicalFiles]) {
    const archive = { filesMatching: predicate => Object.fromEntries(Object.entries(layout).filter(([p]) => predicate(p))) };
    const restored = await context.restoreLayers(archive, accepted.project.layers);
    assert.equal(restored.length, 1);
    const output = restored[0].bytes;
    assert.equal(Object.keys(output).length, Object.keys(files).length);
    for (const path of accepted.project.layers[0].entries) {
      assert.equal(path, bare(path), 'Layer entries must survive canonicalized export paths');
      assert.deepEqual(output[path], canonicalFiles[path]);
    }
  }
  console.log(JSON.stringify({ actualPreludeParser: true, actualPreludeRestore: true,
    projectAccepted: true, negativeControlRejected: true,
    originalAndCanonicalLayoutsByteIdentical: true,
    retainedEntries: accepted.project.layers[0].entries.length,
    editorSha256: crypto.createHash('sha256').update(source).digest('hex') }));
})().catch(error => { console.error(error); process.exitCode = 1; });
