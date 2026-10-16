// Stateful browser fixture: exercises the editor without contacting a device.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_PATH});
 const page=await browser.newPage();let latest={},failAppend=false;const calls=[],missions=new Map();const errors=[];
 page.on('pageerror',e=>errors.push(e.message));
 await page.route('**/api/info',r=>r.fulfill({json:{baud:1000000,imu_present:true,mission_running:false}}));
 await page.route('**/api/result',r=>r.fulfill({json:latest}));
 await page.route('**/api/cmd',async route=>{
   const c=route.request().postDataJSON();calls.push(c);latest={T:c.T,rid:c.rid,ok:true};
   const m=missions.get(c.name),offset=c.offset||0;
   switch(c.T){
    case 300:{const names=[...missions.keys()].sort();latest.missions=names.slice(offset,offset+16);latest.next=offset+16<names.length?offset+16:-1;break}
    case 301:assert(!m);missions.set(c.name,{intro:c.intro,steps:[]});break;
    case 302:assert(m);latest={...latest,intro:m.intro,steps:m.steps.slice(offset,offset+8),next:offset+8<m.steps.length?offset+8:-1};break;
    case 303:if(failAppend){latest.ok=false;latest.error='storage_write_failed'}else m.steps.push(c.json);break;
    case 310:assert(missions.has(c.source));if(m)assert.equal(c.overwrite,true);missions.set(c.name,missions.get(c.source));missions.delete(c.source);break;
    case 308:assert(m);latest.status='started';break;
    case 309:missions.delete(c.name);break;
   }
   await route.fulfill({status:202,json:{accepted:true}});
 });
 await page.goto('http://127.0.0.1:8765/#json');await page.locator('#commandCount').waitFor();
 assert.equal(await page.locator('#commandCount').innerText(),'112');
 await page.locator('#commandSearch').fill('732');await page.locator('.command-entry summary').click();
 await page.getByRole('button',{name:'Load command',exact:true}).click();assert.equal(calls.length,0);
 assert.equal(JSON.parse(await page.locator('#jsonInput').inputValue()).T,732);
 const steps=Array.from({length:10},(_,i)=>JSON.stringify({T:51,delay:i}));
 await page.locator('#missionSteps').fill(steps.join('\n'));
 await page.locator('#saveMission').click();await page.waitForFunction(()=>document.getElementById('missionStatus').textContent==='Mission saved and verified.');
 assert.deepEqual(missions.get('demo').steps,steps);assert(![...missions.keys()].some(n=>n.startsWith('draft_')));
 assert(calls.some(c=>c.T===302&&c.offset===8),'read-back must paginate');
 const target=JSON.stringify(missions.get('demo'));let commits=calls.filter(c=>c.T===310).length;
 page.once('dialog',d=>d.dismiss());await page.locator('#saveMission').click();await page.waitForFunction(()=>!document.getElementById('saveMission').disabled);
 assert.equal(calls.filter(c=>c.T===310).length,commits,'cancel overwrite');
 failAppend=true;page.once('dialog',d=>d.accept());await page.locator('#saveMission').click();await page.waitForFunction(()=>document.getElementById('missionStatus').textContent==='storage_write_failed');
 assert.equal(JSON.stringify(missions.get('demo')),target,'failed upload must preserve original');
 assert.equal(calls.filter(c=>c.T===310).length,commits);failAppend=false;
 await page.locator('#missionSteps').fill('{"T":308,"name":"demo"}');const before=calls.length;
 await page.locator('#saveMission').click();await page.waitForFunction(()=>!document.getElementById('saveMission').disabled);assert.equal(calls.length,before,'reject recursion before upload');
 await page.locator('#missionSteps').fill('');await page.locator('#savedMissions').selectOption('demo');await page.locator('#loadMission').click();
 await page.waitForFunction(()=>document.getElementById('missionSteps').value.split('\n').length===10);
 assert.equal(await page.locator('#missionSteps').inputValue(),steps.join('\n'));
 page.once('dialog',d=>d.accept());await page.locator('#runMission').click();await page.waitForFunction(()=>document.getElementById('missionStatus').textContent.startsWith('Mission started'));
 assert(calls.some(c=>c.T===308&&c.loop===1));
 await page.locator('#languageToggle').click();await page.locator('#commandSearch').fill('板载');
 assert.match(await page.locator('.command-entry summary').innerText(),/板载惯性传感器/);
 await page.setViewportSize({width:390,height:844});await page.screenshot({path:'.pio/json-tools-zh-mobile.png',fullPage:true});
 assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 await page.locator('#languageToggle').click();await page.setViewportSize({width:1440,height:1000});await page.screenshot({path:'.pio/json-tools-en-desktop.png',fullPage:true});
 assert.deepEqual(errors,[]);await browser.close();console.log('PASS catalog, paginated mission save/read, overwrite cancellation, failed upload preservation, validation, run, translation and layouts');
})().catch(e=>{console.error(e);process.exit(1)});
