import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {instantiate} from '../dist/wasm.js';
test('shared demo has four audible stereo tracks and preserves existing tape',async()=>{
 const m=await WebAssembly.compile(readFileSync(new URL('../dist/engine.wasm',import.meta.url))),e=instantiate(m);
 e.tape_init(8000);assert.equal(e.tape_action(13),1);
 let status=new Float32Array(e.memory.buffer,e.tape_status(),1552);
 // Status publishes during rendering; loading never starts transport.
 e.tape_render(128);status=new Float32Array(e.memory.buffer,e.tape_status(),1552);
 assert.equal(status[1],0);assert.equal(status[6],112);assert.ok(status[4]/8000>30&&status[4]/8000<40);
 for(let t=0;t<4;t++)assert.ok(status[12+t]>240000);
 assert.equal(e.tape_action(13),0);e.tape_action(0);let energy=0;
 for(let i=0;i<40;i++){e.tape_render(128);for(const v of new Float32Array(e.memory.buffer,e.tape_output(),256)){assert.ok(Number.isFinite(v));energy+=v*v;}}
 assert.ok(energy>.1);
});

test('New before audio cancels startup demo; project open cancels it before initialization',async()=>{
 const {default:vm}=await import('node:vm');
 const source=readFileSync(new URL('../dist/app.js',import.meta.url),'utf8');
 const clear=source.match(/async function clearProject\(\)\{[^\n]+?\}\n/)[0];
 const state={demoPending:true,status:new Float32Array(1552),project:{params:{tempo:120},name:'Neon Transit'},dirty:false};
 const c=vm.createContext({state,factoryEpoch:0,updateTransport(){},syncControls(){},notice(){}});
 vm.runInContext(clear+';this.clear=clearProject',c);await c.clear();assert.equal(state.demoPending,false);assert.equal(state.project.name,'Untitled');assert.equal(state.status[12],0);
 const apply=source.split('\n').find(l=>l.startsWith('async function applyProject('));
 const stop=Error('stop after initialization boundary');let checked=false;
 const opening=vm.createContext({state:{demoPending:true},factoryEpoch:0,validateProject(){},enableAudio:async()=>{assert.equal(opening.state.demoPending,false);checked=true;throw stop;}});
 vm.runInContext(apply+';this.apply=applyProject',opening);await assert.rejects(opening.apply({}),e=>e===stop);assert.ok(checked);
});
