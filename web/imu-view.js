// Six-axis visualization only: relative yaw drifts without a magnetic reference.
class ImuAttitude {
  constructor(){this.q=[1,0,0,0];this.count=null;this.yawZero=0;}
  update(s){
    const a=s.accel_g,g=s.gyro_dps;
    if(!Array.isArray(a)||!Array.isArray(g)||a.length!==3||g.length!==3||![...a,...g].every(Number.isFinite)||!Number.isInteger(s.sample_count))return false;
    const n=Math.hypot(...a),delta=this.count===null?0:((s.sample_count-this.count)&0xffffff);
    if(this.count!==null&&delta===0)return false;
    if(n<0.05)return false;
    if(this.count===null||delta/112.1>0.5){
      const r=Math.atan2(-a[1],-a[2])/2,p=Math.atan2(a[0],Math.hypot(a[1],a[2]))/2;
      this.q=[Math.cos(r)*Math.cos(p),Math.sin(r)*Math.cos(p),Math.cos(r)*Math.sin(p),-Math.sin(r)*Math.sin(p)];
    }else{
      let [w,x,y,z]=this.q;
      const v=[2*(x*z-w*y),2*(w*x+y*z),w*w-x*x-y*y+z*z];
      const m=a.map(v=>-v/n),gain=Math.abs(n-1)<0.2?2:0;
      const rate=g.map(v=>v*Math.PI/180);
      rate[0]+=gain*(m[1]*v[2]-m[2]*v[1]);rate[1]+=gain*(m[2]*v[0]-m[0]*v[2]);rate[2]+=gain*(m[0]*v[1]-m[1]*v[0]);
      const dt=delta/112.1,steps=Math.ceil(dt/0.01),h=dt/steps/2;
      for(let i=0;i<steps;i++){
        [w,x,y,z]=this.q;const [gx,gy,gz]=rate;
        const q=[w+(-x*gx-y*gy-z*gz)*h,x+(w*gx+y*gz-z*gy)*h,y+(w*gy-x*gz+z*gx)*h,z+(w*gz+x*gy-y*gx)*h];
        const norm=Math.hypot(...q);this.q=q.map(v=>v/norm);
      }
    }
    this.count=s.sample_count;return true;
  }
  angles(){const [w,x,y,z]=this.q;return [Math.atan2(2*(w*x+y*z),1-2*(x*x+y*y)),Math.asin(Math.max(-1,Math.min(1,2*(w*y-z*x)))),Math.atan2(2*(w*z+x*y),1-2*(y*y+z*z))];}
  rotate([a,b,c]){const [w,x,y,z]=this.q;return [(1-2*y*y-2*z*z)*a+2*(x*y-w*z)*b+2*(x*z+w*y)*c,2*(x*y+w*z)*a+(1-2*x*x-2*z*z)*b+2*(y*z-w*x)*c,2*(x*z-w*y)*a+2*(y*z+w*x)*b+(1-2*x*x-2*y*y)*c];}
}
const imuPose=new ImuAttitude();let imuLive=false,imuTimer,imuGeneration=0,imuLast=0;
function drawImu(pose=imuPose,prefix='imu'){
  const canvas=$(prefix+'Canvas'),ctx=canvas.getContext('2d'),width=canvas.clientWidth||600,height=320,dpr=Math.min(devicePixelRatio||1,2);
  if(canvas.width!==Math.round(width*dpr)||canvas.height!==Math.round(height*dpr)){canvas.width=Math.round(width*dpr);canvas.height=Math.round(height*dpr);}
  ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,width,height);
  const scale=Math.min(width/5,100),yaw=-pose.yawZero;
  function project(p){let [x,y,z]=pose.rotate(p);[x,y]=[x*Math.cos(yaw)-y*Math.sin(yaw),x*Math.sin(yaw)+y*Math.cos(yaw)];return [width/2+scale*(x-y)*0.8,height/2+scale*((x+y)*0.35-z*0.9),x+y+z];}
  const points=[[-1,-.65,-.08],[1,-.65,-.08],[1,.65,-.08],[-1,.65,-.08],[-1,-.65,.08],[1,-.65,.08],[1,.65,.08],[-1,.65,.08]].map(project);
  const faces=[[0,1,2,3],[0,4,5,1],[1,5,6,2],[2,6,7,3],[3,7,4,0],[4,7,6,5]];
  faces.sort((a,b)=>a.reduce((s,i)=>s+points[i][2],0)-b.reduce((s,i)=>s+points[i][2],0));
  for(const face of faces){ctx.beginPath();face.forEach((i,j)=>ctx[j?'lineTo':'moveTo'](points[i][0],points[i][1]));ctx.closePath();ctx.fillStyle=face.includes(4)&&face.includes(6)?'#245ddd':'#91aed8';ctx.fill();ctx.strokeStyle='#173c72';ctx.stroke();}
  const origin=project([0,0,.12]);ctx.font='bold 14px system-ui';
  for(const [p,label,color] of [[[1.6,0,.12],'X','#c73e43'],[[0,1.3,.12],'Y','#07836f'],[[0,0,1.2],'Z','#794ad0']]){const end=project(p);ctx.strokeStyle=color;ctx.beginPath();ctx.moveTo(...origin.slice(0,2));ctx.lineTo(...end.slice(0,2));ctx.stroke();ctx.fillStyle=color;ctx.fillText(label,end[0]+5,end[1]);}
  const angles=pose.angles();angles[2]-=pose.yawZero;
  ['Roll','Pitch','Yaw'].forEach((id,i)=>$(prefix+id).textContent=((angles[i]*180/Math.PI+540)%360-180).toFixed(1)+'°');
}
function stopImu(){imuLive=false;++imuGeneration;clearTimeout(imuTimer);$('imuLive').textContent=t('开始实时显示');$('imuLive').setAttribute('aria-pressed','false');}
async function readImuView(token,live=false){
  let s;
  if(live){
    const response=await fetch('/api/imu',{cache:'no-store',signal:AbortSignal.timeout(2500)});
    if(!response.ok)throw Error('HTTP '+response.status);
    s=await response.json();
    if(token!==imuGeneration)return;
    $('imuResult').textContent=JSON.stringify(s,null,2);
    if(s.ok===false)throw Error(s.error||'IMU read failed');
  }else s=await send({T:740},$('imuResult'));
  if(token!==imuGeneration)return;
  if(!imuPose.update(s))return;
  imuLast=performance.now();$('imuResult').textContent=JSON.stringify(s,null,2);$('imuStatus').textContent=t('姿态已更新');drawImu();
}
async function pollImu(){
  if(!imuLive)return;
  if(document.hidden||$('imu').hidden){stopImu();return;}
  const token=imuGeneration;
  if(!active){try{await readImuView(token,true)}catch(e){if(token!==imuGeneration)return;$('imuStatus').textContent=e.message;if(!['imu_data_not_ready','imu_no_fresh_sample'].includes(e.message)){stopImu();return;}}}
  if(imuLive&&token===imuGeneration)imuTimer=setTimeout(pollImu,100);
}
$('imuLive').onclick=()=>{if(imuLive){stopImu();$('imuStatus').textContent=t('实时显示已暂停');return;}imuPose.count=null;imuLive=true;++imuGeneration;$('imuLive').textContent=t('暂停实时显示');$('imuLive').setAttribute('aria-pressed','true');pollImu();};
$('readImu').onclick=()=>readImuView(imuGeneration).catch(e=>{$('imuStatus').textContent=e.message;report(e)});
$('imuZero').onclick=()=>{imuPose.yawZero=imuPose.angles()[2];drawImu();};
new MutationObserver(()=>{if($('imu').hidden&&imuLive){stopImu();$('imuStatus').textContent=t('实时显示已暂停')}else if(!$('imu').hidden)drawImu()}).observe($('imu'),{attributes:true,attributeFilter:['hidden']});
document.addEventListener('visibilitychange',()=>{if(document.hidden&&imuLive){stopImu();$('imuStatus').textContent=t('实时显示已暂停')}});
window.addEventListener('resize',()=>{if(!$('imu').hidden)drawImu()});
setInterval(()=>{if(imuLive&&performance.now()-imuLast>1500)$('imuStatus').textContent=t('等待新数据，姿态已冻结')},500);
drawImu();
