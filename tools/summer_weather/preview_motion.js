// Browser counterpart of MMSummerAtmosphereState; checked against native samples.
(() => {
  const add=(a,b)=>a.map((v,i)=>v+b[i]), sub=(a,b)=>a.map((v,i)=>v-b[i]);
  const mul=(a,s)=>a.map(v=>v*s), dot=(a,b)=>a.reduce((v,x,i)=>v+x*b[i],0);
  const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
  const smooth=x=>{x=clamp(x,0,1);return x*x*(3-2*x);};
  const view={eye:[0,100,0],forward:[0,0,1],right:[-1,0,0],up:[0,1,0],tanHalfFov:0.5773503,aspect:16/9};
  const nightWeight=hour=>{hour=(hour+24)%24;if(hour<5||hour>=19)return 1;if(hour<7)return 1-smooth((hour-5)/2);if(hour<17)return 0;return smooth((hour-17)/2);};
  const moteCount=32,fireflyCount=96;
  class Motion {
    constructor(){this.particles=Array.from({length:moteCount+fireflyCount},()=>({alpha:0,generation:0}));this.reset();}
    reset(){this.particles.forEach(p=>p.alpha=0);this.beams=[];this.randomState=0x53554d52;this.elapsed=0;this.initialized=false;}
    random(){let x=this.randomState;x^=x<<13;x^=x>>>17;x^=x<<5;this.randomState=x>>>0;return (this.randomState>>>8)/16777216;}
    inView(pos){const r=sub(pos,view.eye),z=dot(r,view.forward);return z>=100&&z<=2600&&Math.abs(dot(r,view.right))<=z*view.tanHalfFov*view.aspect*1.1&&Math.abs(dot(r,view.up))<=z*view.tanHalfFov*1.1;}
    spawn(i){
      const p=this.particles[i];p.kind=i<moteCount?0:1;const ordinal=p.kind?i-moteCount:i;
      const x=-0.90+(ordinal%8+0.2+this.random()*0.6)*(1.80/8);
      const y=p.kind?-0.80+(Math.floor(ordinal/8)+0.2+this.random()*0.6)*(1.10/Math.ceil(fireflyCount/8)):-0.80+(Math.floor(ordinal/8)+0.2+this.random()*0.6)*(1.60/4);
      const tier=ordinal%10,depth=tier<2?200+this.random()*180:tier<6?500+this.random()*550:1200+this.random()*1100;
      p.anchor=add(add(add(view.eye,mul(view.forward,depth)),mul(view.right,x*depth*view.tanHalfFov*view.aspect)),mul(view.up,y*depth*view.tanHalfFov));
      p.position=p.anchor.slice();p.phase=this.random()*6.2831853;p.frequency=0.6+this.random()*0.5;p.orbit=p.kind?6+this.random()*16:2+this.random()*4;
      p.drift=mul(view.right,p.kind?this.random()-0.5:3+this.random()*4);p.drift[1]+=p.kind?0:1+this.random()*2;
      p.radius=clamp(depth*(p.kind?0.0036:0.0045),0.45,p.kind?5.8:10);p.age=0;p.alpha=0;p.generation++;
    }
    step(input={}){
      const {eligible=true,paused=false,seconds=0.05,hour=12,dayVisibility=1,sunbeams=true}=input;
      if(!eligible){this.reset();return;}
      const dt=paused?0:clamp(seconds,0,0.1);this.elapsed=Math.fround(this.elapsed+dt);
      const night=nightWeight(hour),daylight=(1-night)*clamp(dayVisibility,0,1);
      this.particles.forEach((p,i)=>{
        if(!this.initialized)this.spawn(i);
        else if(dt>0){
          p.age=Math.fround(p.age+dt);const angle=p.age*p.frequency+p.phase;
          const hover=[p.orbit*(Math.sin(angle)-Math.sin(p.phase)),p.orbit*0.45*(Math.sin(angle*0.7)-Math.sin(p.phase*0.7)),p.orbit*0.6*(Math.cos(angle*0.8)-Math.cos(p.phase*0.8))];
          p.position=add(add(p.anchor,mul(p.drift,p.age)),hover);if(!this.inView(p.position))this.spawn(i);
        }
        const r=sub(p.position,view.eye),depth=Math.max(100,dot(r,view.forward));
        const x=Math.abs(dot(r,view.right))/(depth*view.tanHalfFov*view.aspect),y=Math.abs(dot(r,view.up))/(depth*view.tanHalfFov);
        const edge=smooth((1.08-Math.max(x,y))/0.18),pulse=0.5+0.5*Math.sin(p.age*p.frequency+p.phase);
        p.alpha=smooth(p.age/0.8)*edge*(p.kind?night*(0.48+0.52*pulse):daylight*(0.58+0.22*pulse));
      });
      this.initialized=true;this.beams=[];
      if(sunbeams&&daylight>0.01){
        const source=[0.45,0.8,-0.25],norm=Math.hypot(...source),sun=mul(source,1/norm);
        for(let index=0;index<3;index++){
          const depth=700+index*520,halfHeight=depth*view.tanHalfFov;
          const top=add(add(add(view.eye,mul(view.forward,depth)),mul(view.up,halfHeight*1.05)),mul(view.right,(-0.58+index*0.60)*halfHeight*view.aspect));
          const length=halfHeight*1.95/sun[1],opacity=0.10*daylight*smooth((sun[1]-0.15)/0.45)*(0.86+0.14*Math.sin(this.elapsed*0.24+index*1.4));
          const vertices=[];
          [0,0.22,0.75,1].forEach((t,row)=>{const width=halfHeight*(0.035+0.12*t),center=sub(top,mul(sun,length*t));for(let column=0;column<3;column++)vertices.push([...add(center,mul(view.right,(column-1)*width)),row>0&&row<3&&column===1?opacity:0]);});
          this.beams.push(vertices);
        }
      }
    }
  }
  // Local quad coordinates follow native billboardMtxF, including screen-right handedness.
  const billboardCorners=(position,radius,v=view)=>[[-1,-1],[1,-1],[1,1],[-1,1]].map(([x,y])=>add(add(position,mul(v.right,x*radius)),mul(v.up,y*radius)));
  globalThis.MMSummerMotion={Motion,view,nightWeight,billboardCorners};
  if(typeof module!=='undefined')module.exports=globalThis.MMSummerMotion;
})();
