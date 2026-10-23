const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const b=await chromium.launch({headless:true,executablePath:process.env.CHROME_PATH});
 const p=await b.newPage();const errors=[];p.on('pageerror',e=>errors.push(e.message));
 await p.goto('http://127.0.0.1:8765/#ttlimu');
 await p.evaluate(()=>{window.originalSend=send;window.calls=0;window.inflight=0;window.peak=0;send=async cmd=>{calls++;peak=Math.max(peak,++inflight);await new Promise(r=>setTimeout(r,50));inflight--;if(window.missing)throw Error('ttl_imu_read_failed');return {T:cmd.T,id:cmd.id,ok:true,accel_raw:[1,-2,3],gyro_raw:[4,5,6],quaternion_xyzw:[0,0,0,1]};};});
 await p.locator('#ttlImuLive').click();await p.waitForTimeout(600);
 assert.equal(await p.evaluate(()=>peak),1);assert((await p.evaluate(()=>calls))>=2);
 await p.locator('#ttlImuLive').click();await p.waitForTimeout(70);
 await p.locator('#ttlImuRead').click();await p.waitForTimeout(90);
 assert.equal(await p.locator('#ttlAccel').innerText(),'1, -2, 3');
 const pose=await p.evaluate(()=>{
  ttlPose.q=[Math.SQRT1_2,Math.SQRT1_2,0,0];drawImu(ttlPose,'ttlImu');
  return {roll:document.getElementById('ttlImuRoll').textContent,onboard:imuPose.q.slice()};
 });assert.equal(pose.roll,'90.0°');assert.deepEqual(pose.onboard,[1,0,0,0]);
 // A pending reply must neither blank the last frame nor reset its freshness deadline.
 await p.evaluate(()=>{window.savedSend=send;send=cmd=>new Promise(resolve=>window.finishRead=()=>resolve({T:751,id:cmd.id,ok:true,accel_raw:[7,8,9],gyro_raw:[1,2,3],quaternion_xyzw:[0,0,Math.SQRT1_2,Math.SQRT1_2]}));});
 await p.locator('#ttlImuRead').click();await p.waitForTimeout(100);
 assert.equal(await p.locator('#ttlAccel').innerText(),'1, -2, 3');
 assert.equal(await p.locator('#ttlImuRoll').innerText(),'90.0°');
 await p.waitForTimeout(1500);assert.equal(await p.locator('#ttlAccel').innerText(),'—');
 assert.equal(await p.locator('#ttlImuCanvas').getAttribute('data-fresh'),'false');
 await p.evaluate(()=>finishRead());await p.waitForTimeout(50);
 assert.equal(await p.locator('#ttlImuYaw').innerText(),'90.0°');
 await p.locator('#ttlImuZero').click();assert.equal(await p.locator('#ttlImuYaw').innerText(),'0.0°');
 await p.evaluate(()=>send=savedSend);

 await p.screenshot({path:'.pio/ttl-imu-desktop.png',fullPage:true});
 await p.waitForTimeout(1550);assert.equal(await p.locator('#ttlAccel').innerText(),'—');
 await p.locator('#ttlImuLive').click();await p.locator('[data-page="overview"]').click();await p.waitForTimeout(100);
 const n=await p.evaluate(()=>calls);await p.waitForTimeout(300);assert.equal(await p.evaluate(()=>calls),n);
 await p.locator('[data-page="ttlimu"]').click();await p.locator('#languageToggle').click();
 await p.setViewportSize({width:390,height:844});await p.screenshot({path:'.pio/ttl-imu-mobile.png',fullPage:true});
 assert(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 await p.locator('#ttlImuRead').click();await p.waitForTimeout(80);
 await p.screenshot({path:'.pio/ttl-imu-mobile.png',fullPage:true});
 await p.evaluate(()=>{window.missing=true});await p.locator('#ttlImuLive').click();await p.waitForTimeout(150);
 assert.equal(await p.locator('#ttlImuLive').getAttribute('aria-pressed'),'false');assert.equal(await p.locator('#ttlAccel').innerText(),'—');
 const background=await p.evaluate(async()=>{
  const previous=socket;const button=ttlIdForm.querySelector('button');let toggles=0;
  const observer=new MutationObserver(r=>toggles+=r.length);observer.observe(button,{attributes:true,attributeFilter:['disabled']});
  socket={readyState:WebSocket.OPEN,send:data=>{const cmd=JSON.parse(data);setTimeout(()=>received({...cmd,ok:true}),30);}};
  try{await originalSend({T:751,id:200},null,{background:true});return {toggles,disabled:button.disabled};}
  finally{observer.disconnect();socket=previous;}
 });assert.deepEqual(background,{toggles:0,disabled:false});
 assert.deepEqual(errors,[]);await b.close();console.log('PASS: TTL IMU sequential reads, stale clearing, navigation, missing device, bilingual mobile layout');
})().catch(e=>{console.error(e);process.exit(1)});
