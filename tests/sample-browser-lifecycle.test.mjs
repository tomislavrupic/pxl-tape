import assert from 'node:assert/strict';
import test from 'node:test';
import {mountSampleBrowser} from '../dist/sample-browser.js';

const tick=()=>new Promise(setImmediate);
const deferred=()=>{let resolve;const promise=new Promise(r=>resolve=r);return {promise,resolve};};
const sound=(asset_id,filename)=>({asset_id,filename,duration_seconds:1,channels:2,sample_rate:48000,relative_path:filename});
const old=sound(1,'old.wav'),fresh=sound(2,'new.wav');
const handle='/audio/'+'a'.repeat(32);

function fixture(t,respond,load=async()=>{},localCookie=false){
  const elements=new Map();
  function element(){return {value:'',children:[],dataset:{},append(...items){this.children.push(...items);},replaceChildren(){this.children=[];},getContext(){return {clearRect(){}};},pause(){},removeAttribute(){},load(){}};}
  const root={querySelector(id){if(!elements.has(id))elements.set(id,element());return elements.get(id);},querySelectorAll(){return root.querySelector('#zogg-results').children;}};
  const get=id=>root.querySelector('#'+id);
  function global(name,value){const previous=Object.getOwnPropertyDescriptor(globalThis,name);Object.defineProperty(globalThis,name,{configurable:true,writable:true,value});t.after(()=>{if(previous)Object.defineProperty(globalThis,name,previous);else delete globalThis[name];});}
  global('document',{createElement:element,documentElement:{dataset:{zoggLocal:localCookie?'true':'false'}}});
  global('location',{hostname:'127.0.0.1',hash:localCookie?'':'#zogg=test',pathname:'/',search:''});
  global('sessionStorage',{setItem(){},getItem(){return 'stale-session-token';},removeItem(){}});global('history',{replaceState(){}});
  const requests=[];
  global('fetch',async(url,options)=>{
    requests.push({url,headers:options.headers});
    const data=options.body?JSON.parse(options.body):{};
    const result=url==='/api/status'?{libraries:[]}:await respond(url,data);
    return {ok:!result?.error,json:async()=>result,arrayBuffer:async()=>new ArrayBuffer(44)};
  });
  let decodes=0;
  get('zogg-track').value='0';
  const browser=mountSampleBrowser({root,load,decode:async()=>{decodes++;throw Error('Unexpected preview decode');},notice(){}});
  return {get,browser,requests,get decodes(){return decodes;},search(){get('zogg-form').onsubmit({preventDefault(){}});}};
}

test('a server-authenticated local window does not send a stale stored bearer token',async t=>{
  const f=fixture(t,async()=>({results:[],total:0}),async()=>{},true);
  await tick();
  assert.ok(f.requests.length>=2);
  for(const request of f.requests)assert.equal(request.headers.Authorization,undefined,'the fresh session cookie must not be overridden by a token from the previous bridge run');
});

test('a new search removes old selectable rows and requires a fresh selection',async t=>{
  const pending=deferred();let searches=0;const prepared=[],loaded=[];
  const f=fixture(t,async(url,data)=>{
    if(url==='/api/search')return ++searches===1?{results:[old],total:1}:pending.promise;
    if(url==='/api/inspect')return {asset:data.asset_id===1?old:fresh};
    if(url==='/api/prepare'){prepared.push(data.asset_id);return {audio:handle,duration:1};}
    return {};
  },async file=>loaded.push(file.name));
  await tick();assert.equal(f.get('zogg-results').children.length,1);
  f.search();
  assert.equal(f.get('zogg-results').children.length,0,'old search results must not remain selectable during a request');
  pending.resolve({results:[fresh],total:1});await tick();
  assert.equal(f.get('zogg-sampler').disabled,true);
  await f.get('zogg-sampler').onclick();assert.deepEqual(prepared,[]);
  await f.get('zogg-results').children[0].onclick();
  await f.get('zogg-sampler').onclick();
  assert.deepEqual(prepared,[2]);assert.deepEqual(loaded,['new.wav']);
});

test('late search responses cannot replace newer results or their selection',async t=>{
  const first=deferred(),second=deferred();let searches=0;
  const f=fixture(t,async url=>{
    if(url==='/api/search')return ++searches===1?{results:[old],total:1}:searches===2?first.promise:second.promise;
    if(url==='/api/inspect')return {asset:fresh};
    return {};
  });
  await tick();f.search();f.search();second.resolve({results:[fresh],total:1});await tick();
  await f.get('zogg-results').children[0].onclick();
  first.resolve({results:[old],total:1});await tick();
  assert.equal(f.get('zogg-results').children[0].children[0].textContent,'new.wav');
  assert.equal(f.get('zogg-name').textContent,'new.wav');
});

test('Stop suppresses a preview that is still preparing',async t=>{
  const pending=deferred();
  const f=fixture(t,async url=>{
    if(url==='/api/search')return {results:[old],total:1};
    if(url==='/api/inspect')return {asset:old};
    if(url==='/api/preview')return pending.promise;
    return {};
  });
  await tick();await f.get('zogg-results').children[0].onclick();
  const preview=f.get('zogg-preview').onclick();f.get('zogg-preview-stop').onclick();
  pending.resolve({audio:handle,duration:1});await preview;
  assert.equal(f.decodes,0);assert.equal(f.get('zogg-audio').src,undefined);
});

test('a failed load restores browser controls and reports the failure',async t=>{
  const f=fixture(t,async url=>{
    if(url==='/api/search')return {results:[old],total:1};
    if(url==='/api/inspect')return {asset:old};
    if(url==='/api/prepare')return {audio:handle,duration:1};
    return {};
  },async()=>{throw Error('Decode failed; current audio preserved');});
  await tick();await f.get('zogg-results').children[0].onclick();await f.get('zogg-sampler').onclick();
  assert.equal(f.get('zogg-sampler').disabled,false);assert.equal(f.get('zogg-search').disabled,false);
  assert.equal(f.get('zogg-status').textContent,'Decode failed; current audio preserved');
});
