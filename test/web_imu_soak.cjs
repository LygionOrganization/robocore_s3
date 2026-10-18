// Seven-minute browser soak against simulated telemetry; no hardware is accessed.
const {chromium}=require('playwright');const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});
 const page=await browser.newPage();let count=0,inflight=0,maximum=0,commands=0;const errors=[];
 page.on('pageerror',e=>errors.push(e.message));
 await page.route('**/api/imu',async route=>{
  ++inflight;maximum=Math.max(maximum,inflight);++count;
  await new Promise(r=>setTimeout(r,count%31===0?400:5));
  await route.fulfill({json:{T:740,ok:true,sample_count:count*12,accel_g:[0,0,-1],gyro_dps:[0,0,0],temperature_c:25}});
  --inflight;
 });
 page.on('request',r=>{if(r.url().endsWith('/api/cmd'))++commands});
 await page.goto('http://127.0.0.1:8765/#imu');await page.locator('#imuLive').click();
 for(let minute=1;minute<=7;++minute){await page.waitForTimeout(60000);assert.equal(await page.locator('#imuLive').getAttribute('aria-pressed'),'true');assert.deepEqual(errors,[]);console.log(`SOAK minute ${minute}: reads=${count}, max_inflight=${maximum}, command_posts=${commands}`)}
 assert.equal(maximum,1);assert.equal(commands,0);assert(count>2000);
 await page.locator('#imuLive').click();await page.waitForTimeout(500);const stopped=count;await page.waitForTimeout(500);assert.equal(count,stopped);
 await browser.close();console.log('PASS seven-minute soak, no request pile-up or command submissions; pause stops reads');
})().catch(e=>{console.error(e);process.exit(1)});
