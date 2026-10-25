import assert from 'node:assert/strict';
import {packDrumKit,KIT_IDS,fetchDrumKit} from '../dist/drum-kit.js';
import {createProject,encodeProject,decodeProject} from '../dist/project.js';
const sounds=KIT_IDS.map((_,k)=>({rate:48000,sample:[new Float32Array(32+k).fill((k+1)/10),new Float32Array(32+k).fill(-(k+1)/20)]}));
const kit=packDrumKit(sounds,48000),slot=kit[0].length/8;
for(let k=0;k<8;k++){assert.equal(kit[0][slot*k],sounds[k].sample[0][0]);assert.equal(kit[1][slot*k],sounds[k].sample[1][0]);assert.equal(kit[0][slot*k+slot-1],0);}
const p=createProject();p.drumKit=kit;p.kitName='Factory Drums';p.params.drumMode=2;p.sequence[8][127]=true;p.sample=[Float32Array.of(.5,.4),Float32Array.of(.3,.2)];
const restored=await decodeProject(await encodeProject(p));assert.deepEqual(restored.drumKit,kit);assert.equal(restored.kitName,p.kitName);assert.equal(restored.sequence[8][127],true);assert.deepEqual(restored.sample,p.sample);
await assert.rejects(fetchDrumKit([],{},()=>{},48000),/incomplete/);
assert.throws(()=>packDrumKit(sounds.slice(1),48000));const bad=sounds.map(p=>({...p,sample:p.sample.map(c=>c.slice())}));bad[4].sample[0][0]=NaN;assert.throws(()=>packDrumKit(bad,48000));
console.log('PASS: distinct stereo pad packing, padding, kit/sampler project roundtrip, lane8 and invalid kit rejection');
