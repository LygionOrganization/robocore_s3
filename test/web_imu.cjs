const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const b=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});
 const p=await b.newPage();const errors=[];p.on('pageerror',e=>errors.push(e.message));
 await p.goto('http://127.0.0.1:8765/#imu');
 await p.evaluate(()=>{
   logs.length=0;receivedResults.clear();
   const result={rid:900,T:400,ok:true,sta_state:'connecting'};
   active={id:900,finish:()=>{active=null}};received(result);received(result);received(result);
   if(logs.length!==1)throw Error('duplicate result logged');
   received({rid:899,T:400,ok:true});if(logs.length!==1)throw Error('unrelated cached result logged');
   active={id:901,finish:()=>{active=null}};received(result);if(active.id!==901)throw Error('old result completed new request');active=null;
 });
 const math=await p.evaluate(()=>{const f=new ImuAttitude();f.update({accel_g:[0,0,-1],gyro_dps:[0,0,0],sample_count:0});for(let i=1;i<=112;i++)f.update({accel_g:[0,0,-1],gyro_dps:[0,0,90],sample_count:i});return f.angles().map(x=>x*180/Math.PI)});
 assert(Math.abs(math[2]-90)<1);assert(Math.abs(math[0])<.01);
 await p.evaluate(()=>{window.requests=0;send=async()=>{window.requests++;return {ok:true,accel_g:[0.5,0,-Math.sqrt(.75)],gyro_dps:[0,0,0],sample_count:window.requests*6}}});
 await p.route('**/api/imu',async r=>{
   const data=await p.evaluate(()=>{requests++;return window.imuMissing?{ok:false,error:'imu_not_present'}:{ok:true,accel_g:[0.5,0,-Math.sqrt(.75)],gyro_dps:[0,0,0],sample_count:requests*6}});
   await r.fulfill({json:data});
 });
 await p.locator('#imuLive').click();await p.waitForTimeout(500);
 assert(Math.abs(parseFloat(await p.locator('#imuPitch').innerText())-30)<1);
 await p.screenshot({path:'.pio/imu-3d-desktop.png',fullPage:true});
 await p.locator('#imuLive').click();const n=await p.evaluate(()=>requests);await p.waitForTimeout(150);assert.equal(await p.evaluate(()=>requests),n);
 await p.locator('#imuLive').click();await p.locator('[data-page="overview"]').click();const k=await p.evaluate(()=>requests);await p.waitForTimeout(150);assert.equal(await p.evaluate(()=>requests),k);
 await p.locator('[data-page="imu"]').click();await p.locator('#languageToggle').click();await p.setViewportSize({width:390,height:844});await p.screenshot({path:'.pio/imu-3d-mobile.png',fullPage:true});
 assert(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 await p.evaluate(()=>{window.imuMissing=true});await p.locator('#imuLive').click();await p.waitForTimeout(100);assert.equal(await p.locator('#imuLive').getAttribute('aria-pressed'),'false');
 assert.deepEqual(errors,[]);await b.close();console.log('PASS: yaw integration, tilt, live/pause/navigation, mobile, missing sensor, no JS errors');
})().catch(e=>{console.error(e);process.exit(1)});
