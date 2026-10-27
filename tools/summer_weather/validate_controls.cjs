const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(process.argv[2],'utf8');
assert(Buffer.byteLength(html)<1000000);assert(!html.includes('__SUMMER_BACKGROUND__'));assert(!/<(?:html|body|head|!doctype)\b/i.test(html));
const controls=[12,18,23].map(hour=>({dataset:{hour:String(hour)},textContent:hour===12?'Day':hour===18?'Dusk':'Night',setAttribute(k,v){this[k]=v;},addEventListener(k,f){this[k]=f;}}));
const beams={checked:false,addEventListener(k,f){this[k]=f;}},play={setAttribute(k,v){this[k]=v;},addEventListener(k,f){this[k]=f;}},caption={};
let frame,photo,textureId=0,currentTexture,vertices=[],draws=[],shaderId=0;
const shaders=[],textures=[];
const gl={VERTEX_SHADER:35633,FRAGMENT_SHADER:35632,COMPILE_STATUS:35713,LINK_STATUS:35714,ARRAY_BUFFER:34962,
FLOAT:5126,TEXTURE_2D:3553,TEXTURE_MIN_FILTER:10241,TEXTURE_MAG_FILTER:10240,LINEAR:9729,TEXTURE_WRAP_S:10242,
TEXTURE_WRAP_T:10243,CLAMP_TO_EDGE:33071,RGBA:6408,UNSIGNED_BYTE:5121,UNPACK_FLIP_Y_WEBGL:37440,BLEND:3042,
COLOR_BUFFER_BIT:16384,SRC_ALPHA:770,ONE_MINUS_SRC_ALPHA:771,DYNAMIC_DRAW:35048,TRIANGLES:4,
createShader(type){const s={id:++shaderId,type};shaders.push(s);return s;},shaderSource(s,source){s.source=source;},compileShader(){},getShaderParameter(){return true;},
createProgram(){return {};},attachShader(){},linkProgram(){},getProgramParameter(){return true;},useProgram(){},createBuffer(){return {};},bindBuffer(){},
getAttribLocation(_,name){return {aPosition:0,aUv:1,aColor:2}[name];},enableVertexAttribArray(){},vertexAttribPointer(){},getUniformLocation(_,name){return name;},uniform1i(){},
createTexture(){const t={id:++textureId};textures.push(t);return t;},bindTexture(_,t){currentTexture=t;},texParameteri(){},
texImage2D(...args){if(args.length===9){currentTexture.width=args[3];currentTexture.height=args[4];currentTexture.rgba=Array.from(args[8]);}else currentTexture.photo=true;},
pixelStorei(){},viewport(){},disable(){},enable(){},clearColor(){},clear(){draws=[];},blendFunc(){},
bufferData(_,data){vertices=Array.from(data);},drawArrays(){draws.push({vertices,texture:currentTexture.id,textured:vertices.length>0});}
};
// Preserve the actual shader's textured/untextured uniform in each draw batch.
let isTextured=true;gl.uniform1i=(key,value)=>{if(key==='uTextured')isTextured=!!value;};
gl.drawArrays=()=>{assert(vertices.every(Number.isFinite),'Projected sprite/beam vertices must be finite.');draws.push({vertices,texture:isTextured?currentTexture.id:null});};
const canvas={clientWidth:1280,dataset:{},getContext(){return gl;}};
const root={querySelector(q){return {'#summer-scene':canvas,'#summer-beams':beams,'#summer-play':play,'#summer-caption':caption}[q];},querySelectorAll(q){assert.equal(q,'[data-hour]');return controls;}};
vm.runInNewContext(html.match(/<script>([\s\S]*)<\/script>/)[1],{
 document:{getElementById(id){assert.equal(id,'summer-weather-preview');return root;}},
 Image:class{constructor(){photo=this;}set src(value){assert(value.startsWith('data:image/jpeg;base64,'));}},
 matchMedia(){return {matches:false};},devicePixelRatio:1,
 ResizeObserver:class{observe(){}},IntersectionObserver:class{constructor(f){this.callback=f;}observe(){this.callback([{isIntersecting:true}]);}},
 requestAnimationFrame(f){frame=f;},console
});
photo.onload();frame(0);frame(100);frame(200);assert(Number(canvas.dataset.frame)>1.25);
play.click();const modes=[];
function capture(name){modes.push({name,width:canvas.width,height:canvas.height,draws:JSON.parse(JSON.stringify(draws))});}
capture('day-clear');assert.equal(draws.length,2);assert.equal(canvas.dataset.beams,'false');
beams.checked=true;beams.change();capture('day-beams');assert.equal(draws.length,3);
beams.checked=false;beams.change();assert.equal(draws.length,2);
assert.equal(canvas.dataset.beams,'false');
const frozen=canvas.dataset.frame;frame(300);frame(400);assert.equal(canvas.dataset.frame,frozen);
controls[2].click();capture('night');assert.equal(canvas.dataset.mode,'night');assert.equal(draws.length,2);
controls[1].click();assert.equal(canvas.dataset.mode,'dusk');assert.equal(controls[1]['aria-pressed'],'true');
play.click();frame(500);frame(600);assert(Number(canvas.dataset.frame)>Number(frozen));
canvas.clientWidth=320;controls[0].click();assert.equal(canvas.width,320);assert.equal(canvas.height,180);
if(process.argv[3])fs.writeFileSync(process.argv[3],JSON.stringify({shaders,textures,modes}));
console.log('PASS actual preview script: Day/Dusk/Night, beam draw toggle, pause/play, continuous clock, narrow canvas, shader/geometry capture');
