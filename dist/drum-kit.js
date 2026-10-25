import {fetchFactorySound} from './factory.js?v=0.7.0';
import {resampleAudio} from './project.js?v=0.7.0';
export const KIT_IDS=['kick-circuit','snare-berg','hat-closed','clap-copper','kick-backroom','perc-molecule','hat-open-dust','cymbal-cinch'];
export const KIT_LABELS=['CIRCUIT KICK','BERG SNARE','CLOSED HAT','COPPER CLAP','BACKROOM KICK','MOLECULE PERC','OPEN HAT','CINCH CYMBAL'];
export function packDrumKit(patches,rate){
 if(patches.length!==8||!Number.isFinite(rate)||rate<1000||rate>192000)throw Error('Eight drum sounds required');
 const sounds=patches.map(p=>resampleAudio(p.sample,p.rate,rate));
 if(sounds.some(a=>a.length!==2||a[0].length!==a[1].length||a[0].length<2||a[0].length>rate*2))throw Error('Invalid drum sound');
 const slot=Math.max(...sounds.map(a=>a[0].length))+2,audio=[new Float32Array(slot*8),new Float32Array(slot*8)];
 sounds.forEach((a,k)=>a.forEach((c,i)=>{for(const v of c)if(!Number.isFinite(v)||Math.abs(v)>32)throw Error('Invalid drum audio');audio[i].set(c,k*slot);}));
 return audio;
}
export async function fetchDrumKit(entries,client,decode,rate){
 const selected=KIT_IDS.map(id=>entries.find(e=>e.id===id));if(selected.some(e=>!e))throw Error('Factory drum catalogue incomplete');
 const patches=[];for(const entry of selected)patches.push(await fetchFactorySound(entry,client,decode));
 return packDrumKit(patches,rate);
}
