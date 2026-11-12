'use strict';
const $ = id => document.getElementById(id);
let context, node, state, ready=false, pressing=Promise.resolve();
let pending=[null,null,null,null], shownPage=-1, browserKey='', startupTimer, pendingKind='track';
const macros=['tempo','energy','evolve','mix'], dragging=new Set();
const pages=[
 [['Tempo','Energy','Evolve','Mix'],['Beats per minute','Pulse, drive and intensity','Source notes → transformed phrases','Solo → Duet → Trio → Float']],
 [['Tone','Character','Space','Level'],['Dark → bright','Soft → distinctive mood texture','Lead send to delay and reverb','Mute → original at 50 → louder']],
 [['Tone','Attack','Length','Level'],['Dark → bright','Plucked → softer onset','Short → sustained','Mute → original at 50 → louder']],
 [['Tone','Swing','Pattern','Level'],['Soft → crisp','Straight → lazy','Choose a rhythmic shape','Mute → original at 50 → louder']],
 [['Third voice','Atmosphere','Space','Level'],['Inner melody balance','Background texture balance','Layer send to delay and reverb','Overall layer level']],
 [['Style','Amount','Wobble','Dust'],['Clean, Vinyl, Tape or Grit','Compression and colour','Pitch movement','Crackle and hiss; zero is quiet']],
 [['Time','Repeats','Tone','Wet'],['Tempo-synced note length','Short echoes → longer trails','Dark → clear repeats','Dry → mood balance at 50 → wet']],
 [['Reverb','Size','Tone','Width'],['Dry → mood balance at 50 → wet','Intimate → spacious','Dark → bright room','Mono → normal at 50 → wide']]
];
function knobText(page,i,v){return page===3&&i===2?['Sparse','Steady','Mood','Broken','Busy'][v]:page===5&&i===0?['Clean','Vinyl','Tape','Grit'][v]:page===6&&i===0?['1/8','Dotted 1/8','1/4'][v]:v;}
const storeKey='satie1-favourite-v1';
const moodNames = ['After Hours','Ibiza Chill','Orbit','Blue Note','Ghost'];
let stored={};
try { stored=JSON.parse(localStorage.getItem(storeKey)||localStorage.getItem('bach1-favourite-v1')||'{}'); } catch { $('notice').textContent='Browser storage is unavailable. Keep will last for this session only.'; }
const canvas=$('screen').getContext('2d'), pixels=canvas.createImageData(240,240);
canvas.fillStyle='#0c1117';canvas.fillRect(0,0,240,240);canvas.fillStyle='#b8e8cd';canvas.font='24px Georgia';canvas.fillText('SATIE-1',68,115);canvas.font='12px system-ui';canvas.fillText('Ready when you are',61,142);
function send(message){if(node)node.port.postMessage(message)}
function tap(button){pressing=pressing.then(async()=>{send({type:'buttons',mask:1<<button});await new Promise(r=>setTimeout(r,65));send({type:'buttons',mask:0});await new Promise(r=>setTimeout(r,65));});}
function encoder(role,n){send({type:'enc',role,n})}
function tuneBrowser(){
 const level=state[14];
 const groups=[...new Set(tunes.map(t=>t.group))];
 $('tuneBrowser').hidden=!level;$('browse').disabled=!!level;
 ['new','home','page','save','fx',...macros].forEach(id=>$(id).disabled=!!level||!!state[27]);
 $('browse').disabled=!!level||!!state[27];
 const key=state.slice(14,18).join(':');if(key===browserKey)return;browserKey=key;
 if(!level)return;
 let choices;
 if(level===1){$('browserPath').textContent='Erik Satie · Choose collection';choices=groups;}
 else{$('browserPath').textContent=groups[state[16]]+' · Choose tune';choices=tunes.filter(t=>t.group===groups[state[16]]).map(t=>t.title);}
 $('browserChoices').replaceChildren(...choices.map((name,i)=>{const b=document.createElement('button');b.textContent=name;b.setAttribute('aria-pressed',String(i===state[17]));b.onclick=()=>{pendingKind='piece';encoder(0,i-state[17]);tap(1);};return b;}));
}
function frame(m){
 if(!ready&&m.satie){ready=true;clearTimeout(startupTimer);$('notice').textContent='';$('startRow').hidden=true;document.querySelectorAll('.instrument button,.instrument select,.instrument input').forEach(el=>el.disabled=false);}
 if(m.fb){for(let i=0;i<m.fb.length;i++){const c=((m.fb[i]&255)<<8)|(m.fb[i]>>>8), p=i*4;pixels.data[p]=(c>>>11)*255/31;pixels.data[p+1]=((c>>>5)&63)*255/63;pixels.data[p+2]=(c&31)*255/31;pixels.data[p+3]=255;}canvas.putImageData(pixels,0,0);}
 if(m.store){stored=Object.fromEntries(Object.entries(m.store).map(([k,v])=>[k,Array.from(v)]));try{localStorage.setItem(storeKey,JSON.stringify(stored));$('notice').textContent='Favourite kept. Recall it from the last page of the guide, or return here later.';}catch{$('notice').textContent='Favourite kept for this session. Browser storage is unavailable.';}}
 if(!m.satie)return;
 state=m.satie;
 if(!state[10]){$('mood').value=state[0];$('source').value=state[1];}
 $('mood').disabled=$('source').disabled=!!state[10]||!!state[14]||!!state[27];tuneBrowser();
 const help=state[27];$('guidePanel').hidden=!help;
 if(help){$('guideTitle').textContent=help+' / '+guidePages.length+' · '+guidePages[help-1].title;
 $('guideCopy').textContent=guidePages[help-1].lines.join(' ');
 $('guidePrevious').disabled=help===1;$('guideNext').textContent=help===guidePages.length?'Recall favourite':'Next';}
 document.documentElement.style.setProperty('--mint',state[0]===2?'#c9c8ff':state[0]===1?'#edbd6e':'#b8e8cd');
 const page=state[18];
 if(page!==shownPage){shownPage=page;pending.fill(null);dragging.clear();$('controlTip').textContent='Drag a dial up or down · use arrow keys to fine-tune';}
 $('page').selectedIndex=page;$('new').textContent=page?'Reset page':'New track';$('new').dataset.code=page?'RESET':'CREATE';
 macros.forEach((name,i)=>{
  const el=$(name);el.min=page===0&&i===0?50:0;el.max=state[23+i];
  $(name+'Label').textContent=pages[page][0][i];el.closest('.macro').querySelector('.hint').textContent=pages[page][1][i];
  if(pending[i]!==null){if(state[19+i]===pending[i])pending[i]=null;else return;}
  if(!dragging.has(name)){el.value=state[19+i];$(name+'Value').value=knobText(page,i,state[19+i]);}
 });
 $('play').textContent=state[8]?'Pause':'Play';$('hold').setAttribute('aria-pressed',String(!!state[9]));
 $('now').textContent=tunes[state[1]].title;
 $('status').textContent=state[10]?'New '+pendingKind+' begins on the next bar.':(state[8]?(state[9]?'Holding this section':'Evolving through the score'):'Paused')+' · '+moodNames[state[0]]+' · '+['Solo','Duet','Trio'][state[12]]+' · Track '+state[6];
 refreshDials();
}
$('start').onclick=async()=>{
 $('start').disabled=true;$('start').textContent='Opening the instrument…';
 try{
  if(context)await context.close();ready=false;
  context=new AudioContext({sampleRate:44100});await context.resume();
  if(context.sampleRate!==44100)throw new Error('This browser cannot run the instrument at its required audio rate. Please try Chrome or Safari.');
  const url=URL.createObjectURL(new Blob([workletCode],{type:'text/javascript'}));
  try{await context.audioWorklet.addModule(url);}finally{URL.revokeObjectURL(url);}
  node=new AudioWorkletNode(context,'amb',{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2]});
  node.onprocessorerror=()=>{$('notice').textContent='The audio engine stopped. Reload this page to restart.';};
  node.port.onmessage=e=>{if(e.data.type==='frame')frame(e.data);};
  const wasm=Uint8Array.from(atob(wasm64),c=>c.charCodeAt(0));
  startupTimer=setTimeout(()=>{if(!ready){$('notice').textContent='Audio has not started. Check your audio output, then try again or open this preview in another browser.';$('start').disabled=false;$('start').textContent='Try again';}},5000);
  node.connect(context.destination);send({type:'load',wasm,store:stored,master:Math.round(4096*Number($('volume').value)/100),seed:1});
 }catch(error){$('notice').textContent=error.message;$('start').disabled=false;$('start').textContent='Try again';if(context)await context.close();}
};
$('volume').oninput=()=>{send({type:'master',value:Math.round(4096*Number($('volume').value)/100)});refreshDials();};
$('mood').onchange=()=>{pendingKind='mood';if(state)encoder(0,Number($('mood').value)-state[0]);refreshDials();};
$('source').onchange=()=>{pendingKind='piece';if(state)encoder(2,Number($('source').value)-state[1]);refreshDials();};
$('page').onchange=()=>{if(state)encoder(1,$('page').selectedIndex-state[18]);};$('home').onclick=()=>tap(6);
$('browse').onclick=()=>tap(1);$('browserBack').onclick=()=>tap(6);$('browserClose').onclick=()=>{for(let i=0;i<state[14];i++)tap(6);};
$('play').onclick=()=>{context.resume();tap(10)};$('new').onclick=()=>{pendingKind='track';tap(4)};$('fx').onclick=()=>tap(0);$('hold').onclick=()=>tap(11);$('save').onclick=()=>tap(7);$('guide').onclick=()=>tap(5);$('guideClose').onclick=()=>tap(6);$('guideNext').onclick=()=>tap(1);$('guidePrevious').onclick=()=>encoder(0,-1);

macros.forEach((name,i)=>{
 const el=$(name);
 el.oninput=()=>{if(state){const next=Number(el.value);$(name+'Value').value=knobText(state[18],i,next);encoder(i+3,next-(pending[i]??state[19+i]));pending[i]=next;}paintDial(el);};
 const tip=()=>{$('controlTip').textContent=$(name+'Hint').textContent;};
 el.addEventListener('focus',tip);el.addEventListener('pointerenter',tip);
});
function paintDial(el){
 const fraction=(Number(el.value)-Number(el.min))/(Number(el.max)-Number(el.min)||1);
 el.closest('.dial-shell').style.setProperty('--angle',(-135+270*fraction)+'deg');
 if(macros.includes(el.id))el.setAttribute('aria-valuetext',String($(el.id+'Value').value));
}
function refreshDials(){
 for(const name of ['mood','source','page']){
  const select=$(name),dial=$(name+'Dial');
  dial.value=select.selectedIndex;dial.disabled=select.disabled;
  dial.setAttribute('aria-valuetext',select.selectedOptions[0]?.textContent||'');
 }
 $('volumeValue').value=$('volume').value+'%';
 document.querySelectorAll('.rotary').forEach(paintDial);
 document.querySelectorAll('.piece-key').forEach(b=>{
  b.disabled=!ready||!!state?.[10]||!!state?.[14]||!!state?.[27];
  b.setAttribute('aria-pressed',String(Number(b.dataset.piece)===(state?.[1]??0)));
 });
}
for(const name of ['mood','source','page']){
 $(name+'Dial').oninput=()=>{const select=$(name);select.selectedIndex=Number($(name+'Dial').value);select.dispatchEvent(new Event('change'));refreshDials();};
}
// Native range inputs retain their keyboard and screen-reader behaviour.
// Pointer gestures use a vertical rotary motion without jumping on first touch.
document.querySelectorAll('.rotary').forEach(el=>{
 let gesture;
 el.addEventListener('pointerdown',e=>{
  if(el.disabled||e.button!==0)return;
  e.preventDefault();el.focus();el.setPointerCapture(e.pointerId);
  gesture={id:e.pointerId,y:e.clientY,value:Number(el.value)};dragging.add(el.id);
 });
 el.addEventListener('pointermove',e=>{
  if(!gesture||gesture.id!==e.pointerId)return;
  const span=Number(el.max)-Number(el.min);
  const next=Math.max(Number(el.min),Math.min(Number(el.max),Math.round(gesture.value+(gesture.y-e.clientY)*span/160)));
  if(Number(el.value)!==next){el.value=next;el.dispatchEvent(new Event('input',{bubbles:true}));}
 });
 const finish=()=>{gesture=null;dragging.delete(el.id);};
 el.addEventListener('pointerup',finish);el.addEventListener('pointercancel',finish);el.addEventListener('lostpointercapture',finish);
});
for(const group of [...new Set(tunes.map(t=>t.group))]){
 const collection=document.createElement('div');
 const heading=document.createElement('div');heading.className='collection-label';heading.textContent=group;
 const keys=document.createElement('div');keys.className='piece-keys';
 tunes.filter(t=>t.group===group).forEach((t,i)=>{
  const button=document.createElement('button');button.className='piece-key';button.dataset.piece=t.id;button.textContent=['I','II','III','IV'][i];button.disabled=true;button.title=t.title;button.setAttribute('aria-label','Play '+t.title);
  button.onclick=()=>{$('source').value=t.id;$('source').dispatchEvent(new Event('change'));};keys.append(button);
 });
 collection.append(heading,keys);$('repertoire').append(collection);
}
const portraitImage=new Image();
portraitImage.onload=()=>{if(!ready)canvas.drawImage(portraitImage,0,0,240,240);};
portraitImage.src=portrait;
refreshDials();
