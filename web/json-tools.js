// Offline JSON reference and transactional mission editor.
function localText(pair){return pair[language]}
function renderCatalog(){
  const query=$('commandSearch').value.toLowerCase().trim();
  const group=$('commandGroup').value;
  const list=$('commandList');list.replaceChildren();
  const matches=commandCatalog.filter(c=>(!group||c.group.en===group)&&(!query||[c.T,c.title.en,c.title.zh,c.note.en,c.note.zh].join(' ').toLowerCase().includes(query)));
  $('commandCount').textContent=String(matches.length);
  for(const c of matches){
    const row=document.createElement('details');row.className='command-entry';
    const title=document.createElement('summary');title.textContent=`T=${c.T} · ${localText(c.title)}`;
    const note=document.createElement('p');note.className='small muted';note.textContent=localText(c.note);
    const code=document.createElement('pre');code.className='result';code.textContent=JSON.stringify(c.sample);
    const actions=document.createElement('div');actions.className='actions';
    for(const [label,fn] of [['填入指令',()=>{$('jsonInput').value=JSON.stringify(c.sample,null,2);$('jsonInput').focus()}],['加入脚本',()=>{$('missionSteps').value+=($('missionSteps').value.trim()?'\n':'')+JSON.stringify(c.sample);$('missionSteps').focus()}]]){
      const b=document.createElement('button');b.textContent=t(label);b.onclick=fn;actions.append(b);
    }
    row.append(title,note,code,actions);list.append(row);
  }
}
for(const group of [...new Set(commandCatalog.map(c=>c.group.en))]){
  const option=document.createElement('option');option.value=group;option.textContent=group;$('commandGroup').append(option);
}
$('commandSearch').oninput=renderCatalog;$('commandGroup').onchange=renderCatalog;
let missionBusy=false;
async function missionAction(fn){
  if(missionBusy)return;missionBusy=true;
  $('missionEditor').querySelectorAll('button,input,textarea,select').forEach(b=>b.disabled=true);
  try{await fn()}catch(e){$('missionStatus').textContent=t(e.message);report(e)}finally{missionBusy=false;$('missionEditor').querySelectorAll('button,input,textarea,select').forEach(b=>b.disabled=false)}
}
function missionName(){const name=$('missionName').value.trim();if(!/^[A-Za-z0-9_-]{1,32}$/.test(name)||name==='boot'||name.startsWith('draft_'))throw Error('任务名称需为 1–32 位字母、数字、下划线或连字符；不能使用 boot 或 draft_ 前缀');return name}
async function readMission(name){
  let offset=0,intro='',steps=[];
  do{const r=await send({T:302,name,offset});if(!Array.isArray(r.steps)||!Number.isInteger(r.next))throw Error('任务读取结果不完整');intro=r.intro;steps.push(...r.steps);if(r.next!==-1&&r.next<=offset)throw Error('任务读取结果不完整');offset=r.next}while(offset!==-1);
  return {intro,steps};
}
async function listMissions(){
  let offset=0,names=[];
  do{const r=await send({T:300,offset});if(!Array.isArray(r.missions)||!Number.isInteger(r.next))throw Error('任务读取结果不完整');names.push(...r.missions);if(r.next!==-1&&r.next<=offset)throw Error('任务读取结果不完整');offset=r.next}while(offset!==-1);
  const select=$('savedMissions');select.replaceChildren();
  const blank=document.createElement('option');blank.value='';blank.textContent=t('选择已保存任务');select.append(blank);
  for(const name of names){const option=document.createElement('option');option.value=name;option.textContent=name;select.append(option)}
  return names;
}
$('refreshMissions').onclick=()=>missionAction(listMissions);
$('loadMission').onclick=()=>missionAction(async()=>{const name=$('savedMissions').value;if(!name)throw Error('请先选择已保存任务');if($('missionSteps').value.trim()&&!confirm(t('替换编辑器中的内容？未保存的修改将丢失。')))return;const data=await readMission(name);$('missionName').value=name.startsWith('draft_')?'recovered':name;$('missionIntro').value=data.intro;$('missionSteps').value=data.steps.join('\n');$('missionStatus').textContent=t('已读取任务；运行前请确认步骤。')});
$('saveMission').onclick=()=>missionAction(async()=>{
  const name=missionName(),intro=$('missionIntro').value.trim();
  if(new TextEncoder().encode(intro).length>96)throw Error('任务说明最多 96 UTF-8 字节');
  const lines=$('missionSteps').value.split(/\r?\n/).map(s=>s.trim()).filter(Boolean);
  if(!lines.length||lines.length>64)throw Error('脚本需要 1–64 行 JSON 指令');
  const steps=lines.map((line,i)=>{let c;try{c=JSON.parse(line)}catch{throw Error(t('JSON 解析失败，行号：')+(i+1))}if(!c||Array.isArray(c)||!Number.isInteger(c.T)||'rid' in c||(c.T>=300&&c.T<=399))throw Error(t('无效任务步骤，行号：')+(i+1));return JSON.stringify(c)});
  const draft='draft_'+Date.now().toString(36)+'_'+Math.random().toString(36).slice(2,6);
  for(const json of steps){if(new TextEncoder().encode(JSON.stringify({T:303,name:draft,json,rid:4294967295})).length>255)throw Error('步骤过长：请缩短 JSON，使上传指令不超过 255 字节')}
  const names=await listMissions(),overwrite=names.includes(name);
  if((overwrite||name==='boot_user')&&!confirm(t(name==='boot_user'?'保存 boot_user 后，主板下次启动会自动循环执行这些步骤。继续？':'覆盖已保存的同名任务？')))return;
  $('missionStatus').textContent=t('正在保存草稿：')+draft;
  await send({T:301,name:draft,intro});
  for(const json of steps)await send({T:303,name:draft,json});
  const check=await readMission(draft);
  if(check.intro!==intro||JSON.stringify(check.steps)!==JSON.stringify(steps))throw Error('草稿校验失败；原任务未替换');
  await send({T:310,source:draft,name,overwrite});
  $('missionStatus').textContent=t('任务保存并校验成功。');await listMissions();$('savedMissions').value=name;
});
$('runMission').onclick=()=>missionAction(async()=>{
  const name=missionName(),interval=Number($('missionInterval').value),loop=Number($('missionLoops').value);
  if(!Number.isInteger(interval)||interval<0||interval>60000||!Number.isInteger(loop)||!(loop===-1||loop>=1&&loop<=10000))throw Error('运行参数无效');
  if(!confirm(t('运行主板中已保存的任务？编辑器中未保存的修改不会执行。任务可能使电机运动。')))return;
  await send({T:308,name,interval,loop});$('missionStatus').textContent=t('任务已启动；可使用JSON 调试中的停止循环按钮退出。');
});
$('deleteMission').onclick=()=>missionAction(async()=>{const name=$('savedMissions').value;if(!name)throw Error('请先选择已保存任务');if(!confirm(t('永久删除所选任务？')))return;await send({T:309,name});await listMissions();$('missionStatus').textContent=t('任务已删除。')});
$('exportMission').onclick=()=>{const blob=new Blob([JSON.stringify({name:$('missionName').value,intro:$('missionIntro').value,steps:$('missionSteps').value},null,2)],{type:'application/json'});const link=document.createElement('a');link.href=URL.createObjectURL(blob);link.download='robocore-mission.json';link.click();setTimeout(()=>URL.revokeObjectURL(link.href),1000)};
$('openImportMission').onclick=()=>$('importMission').click();
$('importMission').onchange=async e=>{try{const file=e.target.files[0];if(!file)return;if(file.size>32768)throw Error('导入文件过大');const data=JSON.parse(await file.text());if(typeof data.name!=='string'||typeof data.intro!=='string'||typeof data.steps!=='string')throw Error('导入文件格式无效');if($('missionSteps').value.trim()&&!confirm(t('替换编辑器中的内容？未保存的修改将丢失。')))return;$('missionName').value=data.name;$('missionIntro').value=data.intro;$('missionSteps').value=data.steps;toast('已导入编辑器，尚未保存到主板')}catch(err){report(err)}finally{e.target.value=''}};
renderCatalog();
