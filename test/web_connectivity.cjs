// UI-only fixtures: never connects to a physical device.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_PATH});
 const page=await browser.newPage();let state='disabled',commands=[];
 await page.route('**/api/info',r=>r.fulfill({json:{baud:1000000,sta_state:state,sta_ip:state==='connected'?'192.168.1.42':'',ap_ip:'192.168.4.1',sta_ssid:'Lab'}}));
 await page.route('**/api/cmd',async r=>{commands.push(r.request().postDataJSON());await r.continue()});
 await page.goto('http://127.0.0.1:8765');
 assert.equal(await page.locator('.topbar #stop').count(),0);
 assert.equal(await page.locator('#json #stop').count(),1);
 await page.locator('[data-page=settings]').click();
 const password=page.locator('[name=sta_password]');await password.fill('example-pass');
 await password.locator('..').locator('button').click();assert.equal(await password.getAttribute('type'),'text');
 await password.locator('..').locator('button').click();assert.equal(await password.getAttribute('type'),'password');
 state='connecting';await page.evaluate(()=>info());assert.match(await page.locator('#wifiStatus').textContent(),/Connecting/);
 state='connected';await page.evaluate(()=>info());assert.match(await page.locator('#wifiStatus').textContent(),/192.168.1.42/);
 state='failed';await page.evaluate(()=>info());assert.match(await page.locator('#wifiStatus').textContent(),/retained/);
 await page.locator('[name=sta_ssid]').fill('Lab');await page.locator('#wifiForm button[type=submit]').click();
 await page.waitForTimeout(600);assert(!await page.locator('#log').textContent().then(s=>s.includes('example-pass')));
 page.on('dialog',d=>d.accept());
 await page.locator('[data-page=encoder]').click();await page.locator('#encoderForms form').last().locator('button').click();await page.waitForTimeout(500);
 assert(commands.some(c=>c.T===720&&c.new_id===2));
 await page.locator('[data-page=servo]').click();await page.locator('#servoForms form').first().locator('button').click();await page.waitForTimeout(500);
 assert(commands.some(c=>c.T===13&&c.old_id===1&&c.new_id===2));
 await page.locator('[data-page=settings]').click();await page.setViewportSize({width:390,height:844});await page.screenshot({path:'.pio/wifi-mobile.png',fullPage:true});
 await browser.close();console.log('PASS STA connecting/connected/IP/failure, password visibility/redaction, ID commands, stop placement');
})().catch(e=>{console.error(e);process.exit(1)});
