const fs=require('node:fs'), assert=require('node:assert/strict');
const {Motion,billboardCorners}=require('./preview_motion.js');
// Native billboard right is -X for this camera. UV u=0 must stay at screen left.
assert.deepEqual(billboardCorners([0,100,1000],10),[[10,90,1000],[-10,90,1000],[-10,110,1000],[10,110,1000]]);
const samples=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
const motion=new Motion();let maxPosition=0,maxAlpha=0,maxBeam=0,maxRadius=0;
for(const frame of samples){
  motion.step(frame.input);
  assert.equal(motion.beams.length,frame.beams.length);
  frame.particles.forEach((p,i)=>{
    const actual=motion.particles[i];
    maxRadius=Math.max(maxRadius,Math.abs(p.radius-actual.radius));
    assert.equal(p.kind,actual.kind);
    p.position.forEach((x,j)=>maxPosition=Math.max(maxPosition,Math.abs(x-actual.position[j])));
    maxAlpha=Math.max(maxAlpha,Math.abs(p.alpha-actual.alpha));
  });
  frame.beams.forEach((beam,i)=>beam.forEach((v,j)=>v.forEach((x,k)=>maxBeam=Math.max(maxBeam,Math.abs(x-motion.beams[i][j][k])))));
}
assert(maxPosition<0.02,`Preview flight differs from native by ${maxPosition}`);
assert(maxRadius<0.00001,`Preview sprite radius differs from native by ${maxRadius}`);
assert(maxAlpha<0.0001,`Preview pulse differs from native by ${maxAlpha}`);
assert(maxBeam<0.02,`Preview shafts differ from native by ${maxBeam}`);
console.log(`PASS native/preview parity: ${samples.length} frames; maximum position difference ${maxPosition.toFixed(6)}, opacity ${maxAlpha.toFixed(6)}, radius ${maxRadius.toFixed(6)}, shaft ${maxBeam.toFixed(6)}`);
