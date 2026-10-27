// Offline WebGL review artifact checks. Optional capture is simulated, never gameplay.
const {chromium}=require(process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES+'/playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const [preview,output,record]=process.argv.slice(2);
(async()=>{
 fs.mkdirSync(output,{recursive:true});
 const browser=await chromium.launch({headless:true,args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const context=await browser.newContext({viewport:{width:1180,height:780},deviceScaleFactor:1,...(record?{recordVideo:{dir:output,size:{width:1180,height:780}}}:{})});
 const page=await context.newPage(),errors=[],external=[];
 page.on('pageerror',e=>errors.push(e.message));page.on('request',r=>{if(!/^(file|data):/.test(r.url()))external.push(r.url());});
 await page.goto('file://'+path.resolve(preview));
 const canvas=page.locator('#summer-scene');await canvas.locator('xpath=..').waitFor();
 assert.equal(await canvas.getAttribute('data-mode'),'day');
 assert.equal(await page.locator('#summer-beams').isChecked(),false);
 await page.waitForTimeout(900);await page.locator('#summer-play').click();
 const stopped=await canvas.getAttribute('data-frame');await page.waitForTimeout(180);
 assert.equal(await canvas.getAttribute('data-frame'),stopped,'Pause freezes the actual simulation.');
 const clear=await canvas.screenshot({path:path.join(output,'summer-day.png')});
 await page.locator('#summer-beams').check();
 const rays=await canvas.screenshot({path:path.join(output,'summer-day-beams.png')});
 assert(!clear.equals(rays),'Beam comparison must change rendered pixels.');
 await page.getByRole('button',{name:'Night',exact:true}).click();
 const night=await canvas.screenshot({path:path.join(output,'summer-night.png')});
 assert(!night.equals(clear),'Night must have a distinguishable rendered result.');
 await page.getByRole('button',{name:'Dusk',exact:true}).click();
 assert.equal(await canvas.getAttribute('data-mode'),'dusk');
 await page.locator('#summer-play').click();const before=await canvas.getAttribute('data-frame');await page.waitForTimeout(250);
 assert(Number(await canvas.getAttribute('data-frame'))>Number(before),'Controls preserve active motion.');
 await page.setViewportSize({width:360,height:780});await page.waitForTimeout(120);
 assert(!await page.locator('#summer-weather-preview').evaluate(el=>el.scrollWidth>el.clientWidth+1),'Small-screen control layout fits.');
 await page.screenshot({path:path.join(output,'summer-mobile.png')});
 if(record){
  await page.setViewportSize({width:1180,height:780});
  await page.locator('#summer-beams').uncheck();await page.getByRole('button',{name:'Day',exact:true}).click();await page.waitForTimeout(3500);
  await page.locator('#summer-beams').check();await page.waitForTimeout(3500);
  await page.getByRole('button',{name:'Dusk',exact:true}).click();await page.waitForTimeout(2500);
  await page.getByRole('button',{name:'Night',exact:true}).click();await page.waitForTimeout(4500);
 }
 assert.deepEqual(errors,[]);assert.deepEqual(external,[]);
 const video=page.video();await context.close();if(video)fs.writeFileSync(path.join(output,'video-path.txt'),await video.path());
 await browser.close();
 console.log('PASS preview: offline WebGL, default-off beams, rendered beam/day/night differences, dusk, pause/play, continuous motion, 360px controls, no script errors or network requests');
})().catch(e=>{console.error(e);process.exit(1)});
