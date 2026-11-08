// SPDX-License-Identifier: GPL-3.0-only
// Exercise the real worklet and WASM together, including its published UI state.
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const messages=[];let Processor;
const scope={currentTime:0,performance,console,WebAssembly,Uint8Array,Uint16Array,Float32Array,DataView,BigInt,Proxy,
 AudioWorkletProcessor:class{constructor(){this.port={postMessage:m=>messages.push(m)};}},
 registerProcessor:(name,p)=>{Processor=p;}};
vm.createContext(scope);vm.runInContext(fs.readFileSync('web/emu/worklet.js','utf8'),scope);
const worklet=new Processor();
await worklet.onMessage({type:'load',wasm:fs.readFileSync('build/satie/satie.wasm'),store:{},master:1600,seed:1});
let peak=0;
function render(ms){for(let i=0;i<Math.ceil(ms*44.1/128);i++){
 const output=[new Float32Array(128),new Float32Array(128)];
 scope.currentTime+=128/44100;assert.equal(worklet.process([], [output]),true);
 for(const channel of output)for(const x of channel){assert(Number.isFinite(x));peak=Math.max(peak,Math.abs(x));}
}}
function state(){return messages.filter(m=>m.type==='frame').at(-1).satie;}
function button(n){worklet.onMessage({type:'buttons',mask:1<<n});render(80);worklet.onMessage({type:'buttons',mask:0});render(80);}
function enc(role,n){worklet.onMessage({type:'enc',role,n});render(100);}
render(4000);assert.equal(state()[0],2);assert.equal(state()[1],0);assert.equal(state()[2],60);
assert(messages.some(m=>m.fb?.length===57600));assert(peak>0.005&&peak<1);
button(1);assert.equal(state()[14],1);enc(0,2);button(1);assert.equal(state()[14],2);
assert.equal(state()[16],2);enc(0,3);button(1);render(5000);assert.equal(state()[1],9);assert.equal(state()[14],0);
// Every page publishes the correct ranges, changes controls and keeps playback running.
const seed=state()[6];
for(let page=1;page<8;page++){
 enc(1,1);assert.equal(state()[18],page);assert.equal(state().length,28);
 for(let k=0;k<4;k++){
  const max=state()[23+k];enc(3+k,200);assert.equal(state()[19+k],max);
 }
 render(600);assert.equal(state()[8],1);assert.equal(state()[6],seed);
 button(4);assert.equal(state()[6],seed); // Reset page, never New track.
}
button(6);assert.equal(state()[18],0);
button(5);assert.equal(state()[27],1);
const helpMix=state()[5];enc(6,30);assert.equal(state()[5],helpMix);
enc(0,200);assert.equal(state()[27],8);enc(1,-200);assert.equal(state()[27],1);
button(6);assert.equal(state()[27],0);assert.equal(state()[18],0);
button(10);render(200);assert.equal(state()[8],0);
const html=fs.readFileSync('build/satie/SATIE-1.html','utf8');
assert(!/Goldberg|BWV|Sonata|Partita|BACH-1/.test(html.replace(/const wasm64 = '[^']*';/,'')));
new vm.Script(html.slice(html.indexOf('<script>')+8,html.indexOf('</script>')));
console.log(`SATIE-1 worklet: audio peak ${peak.toFixed(3)}, screen frames, Orbit default, Ogive picker, pause, eight pages, edit ranges, reset and page script passed.`);
