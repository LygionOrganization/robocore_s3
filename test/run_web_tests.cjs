// Start python tools/preview_web.py first. Requires Playwright; optionally set CHROME_PATH.
// Responses are simulated; this test never connects to hardware.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});
 const page=await browser.newPage(); const errors=[]; page.on('pageerror',e=>errors.push(e.message));
 await page.goto('http://127.0.0.1:8765'); await page.waitForTimeout(400);
 assert.equal(await page.locator('html').getAttribute('lang'),'en');
 for(const id of ['overview','devices','node','stepper','encoder','servo','imu','json','settings']){
   await page.locator(`[data-page="${id}"]`).click();
   const text=await page.locator('main').innerText();
   assert(!/[\u4e00-\u9fff]/.test(text.replace('中文','')),id+' untranslated: '+text);
 }
 await page.locator('[data-page="stepper"]').click();
 await page.locator('#stepperForms input[name="id"]').first().fill('17');
 await page.locator('#languageToggle').click(); await page.waitForTimeout(50);
 assert.equal(await page.locator('html').getAttribute('lang'),'zh-CN');
 assert.equal(await page.locator('#stepperForms input[name="id"]').first().inputValue(),'17');
 assert.equal(await page.locator('#stepper h1').innerText(),'精准定位，连续运动');
 assert.equal(await page.locator('#crumb').innerText(),'步进驱动');
 await page.reload();await page.waitForTimeout(100);
 assert.equal(await page.locator('html').getAttribute('lang'),'zh-CN');
 await page.locator('#languageToggle').click();
 await page.locator('[data-page="devices"]').click();await page.locator('#scanForm button').click();
 await page.locator('#ids button').first().waitFor();
 assert.equal(await page.locator('#deviceStat').textContent(),'3 devices');
 await page.locator('#ids button').first().click();
 assert.match(await page.locator('#toast').textContent(),/Selected ID 1/);
 await page.locator('#languageToggle').click(); await page.waitForTimeout(50);
 assert.equal(await page.locator('#deviceStat').textContent(),'3 个');
 await page.locator('[data-page="stepper"]').click();
 page.once('dialog',async d=>{assert.match(d.message(),/确认更改工作模式/);await d.dismiss()});
 await page.locator('#stepperForms form').first().locator('button').click();
 await page.locator('#languageToggle').click();
 page.once('dialog',async d=>{assert.match(d.message(),/Change operating mode/);await d.dismiss()});
 await page.locator('#stepperForms form').first().locator('button').click();
 await page.locator('[data-page="overview"]').click();
 await page.setViewportSize({width:1440,height:1000});await page.screenshot({path:'.pio/web-en-desktop.png',fullPage:true});
 await page.locator('[data-page="servo"]').click();
 assert.equal(await page.locator('#servoForms > section').count(),3);
 for(const [index,title] of ['STS / SMS','HLS','SCS'].entries()){
   const group=page.locator('#servoForms > section').nth(index);
   assert.equal(await group.locator(':scope > h2').textContent(),title);
   assert.equal(await group.locator('form').count(),3);
 }
 await page.screenshot({path:'.pio/servo-grouped-desktop.png',fullPage:true});
 await page.setViewportSize({width:390,height:844});
 for(const lang of ['en','zh']){
   if(lang==='zh')await page.locator('#languageToggle').click();
   for(const id of ['overview','devices','node','stepper','encoder','servo','imu','json','settings']){
     await page.locator(`[data-page="${id}"]`).click();
     assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),`${lang}/${id} overflow`);
   }
   await page.locator('[data-page="overview"]').click();await page.screenshot({path:`.pio/web-${lang}-mobile.png`,fullPage:true});
 }
 assert.deepEqual(errors,[]);
 await browser.close(); console.log('PASS bilingual pages, dynamic scan/status, persisted language, form state, confirmation dialogs and mobile layouts');
})().catch(e=>{console.error(e);process.exit(1)});

