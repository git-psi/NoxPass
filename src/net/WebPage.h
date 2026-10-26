// WebPage.h : the website page (generated from page.html). A single string, served from flash.
#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>NoxPass Bridge</title>
<style>
/* Two colors only. mid/dim are fixed blends of fg and bg, like on the stick. */
:root{--bg:#000;--fg:#fff;--mid:#8c8c8c;--dim:#2e2e2e;--r:6px}
*{box-sizing:border-box}
html,body{margin:0;height:100%;background:var(--bg);overflow:hidden}
*{scrollbar-width:thin;scrollbar-color:var(--fg) var(--bg)}
::-webkit-scrollbar{width:10px;height:10px}
::-webkit-scrollbar-track{background:var(--bg)}
::-webkit-scrollbar-thumb{background:var(--fg);border:2px solid var(--bg);border-radius:var(--r)}
body{color:var(--fg);font:14px/1.3 "Courier New",Courier,monospace;overflow:hidden}
button,input,select{font-family:inherit}
.mid{color:var(--mid)}
.app{display:flex;flex-direction:column;height:100vh;height:var(--h,100vh);max-width:960px;margin:0 auto;padding:0 20px}
h1{flex:none;display:flex;align-items:center;justify-content:center;gap:12px;font-size:24px;font-weight:700;letter-spacing:2px;margin:0;padding:18px 0 12px}
.logo{width:40px;height:40px;flex:none;background:currentColor;-webkit-mask:url(/logo.svg) center/contain no-repeat;mask:url(/logo.svg) center/contain no-repeat}
.frame{flex:1;min-height:0;display:grid;grid-template-columns:1fr 1.15fr;grid-template-rows:minmax(0,1fr);border-top:2px solid var(--fg)}
aside{border-right:2px solid var(--fg);padding:18px 20px 12px 0;display:flex;flex-direction:column;min-height:0}
#list{flex:1;overflow:auto;margin-top:12px}
section{padding:18px 20px 12px 24px;overflow:auto;min-height:0}
.rule{flex:none;height:2px;background:var(--fg)}
.rule.busy{background:var(--dim)}
.rule i{display:block;height:100%;width:0;background:var(--fg);transition:width .5s}
#bar{flex:none;display:flex;justify-content:space-between;align-items:center;gap:12px;padding:12px 0 calc(12px + env(safe-area-inset-bottom))}
#msg{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
#btns{display:flex;gap:8px;flex:none}
.busy-lock{pointer-events:none;opacity:.6}
.r{display:flex;gap:8px;align-items:center}
.in,.btn{height:38px;border:1px solid var(--fg);border-radius:var(--r);background:var(--bg);color:var(--fg);font-size:14px;line-height:1}
.in{width:100%;padding:0 12px;min-width:0}
.in::placeholder{color:var(--mid)}
.in:disabled{color:var(--mid);text-decoration:line-through;border-color:var(--dim)}
:focus-visible{outline:2px solid var(--mid);outline-offset:2px}
.btn{display:inline-flex;align-items:center;justify-content:center;padding:0 14px;cursor:pointer;white-space:nowrap;flex:none}
.btn:hover:not(:disabled){background:var(--fg);color:var(--bg)}
.btn:disabled{border-color:var(--dim);color:var(--dim);cursor:default}
.sq{width:38px;padding:0;font-size:20px;font-weight:700}
.item{display:flex;justify-content:space-between;align-items:center;gap:8px;height:40px;padding:0 12px;margin-bottom:8px;border:1px solid var(--fg);border-radius:var(--r);font-weight:700;cursor:pointer}
.item>span:first-child{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.item.sel{background:var(--fg);color:var(--bg)}
.item.del>span:first-child{text-decoration:line-through;color:var(--mid)}
.item.static{cursor:default}
.badge{display:inline-flex;align-items:center;height:20px;font-size:11px;font-weight:400;border:1px solid var(--mid);color:var(--mid);padding:0 6px;border-radius:4px;flex:none;line-height:1}
.item.sel .badge{border-color:var(--bg);color:var(--bg)}
#setrow{flex:none;border-top:1px solid var(--dim);padding-top:10px;margin-top:4px}
#setrow .item{margin-bottom:0}
.title{font-size:16px;font-weight:700;height:42px}
.field{margin-top:16px}
.field .t{width:150px;flex:none}
.field .t.full{width:100%}
.sel-wrap{position:relative}
.sel-wrap::after{content:"";position:absolute;right:12px;top:50%;margin-top:-2px;border:5px solid transparent;border-top-color:var(--fg);border-bottom:0;pointer-events:none}
.sel-wrap select{appearance:none;-webkit-appearance:none;padding-right:28px;cursor:pointer}
option{background:var(--bg);color:var(--fg)}
.actions{display:flex;justify-content:space-between;margin-top:22px}
h2{font-size:16px;font-weight:700;margin:0 0 4px}
.set{display:flex;justify-content:space-between;align-items:center;gap:16px;padding:14px 0;border-bottom:1px solid var(--dim)}
.set:last-child{border-bottom:0}
.grp{margin:22px 0 0;font-size:12px;letter-spacing:2px;text-transform:uppercase;color:var(--mid);font-weight:700}
.count{margin:10px 2px 0;font-size:12px}
.hint{color:var(--mid);margin:6px 0 0;line-height:1.5}
:focus-visible{outline:2px solid var(--fg);outline-offset:2px}
.set small{display:block;color:var(--mid);font-size:12px;margin-top:2px}
.set .sel-wrap{width:180px;flex:none}
.rng{display:flex;align-items:center;gap:10px;width:180px;flex:none}
.rng output{width:40px;text-align:right;font-size:13px}
.rng input{flex:1;min-width:0;height:26px;margin:0;background:transparent;-webkit-appearance:none;appearance:none;cursor:pointer}
.rng input::-webkit-slider-runnable-track{height:8px;border:1px solid var(--fg);border-radius:var(--r);background:var(--bg)}
.rng input::-moz-range-track{height:6px;border:1px solid var(--fg);border-radius:var(--r);background:var(--bg)}
.rng input::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:22px;margin-top:-8px;border:0;border-radius:4px;background:var(--fg)}
.rng input::-moz-range-thumb{width:16px;height:22px;border:0;border-radius:4px;background:var(--fg)}
.rng input:focus-visible{outline:2px solid var(--mid);outline-offset:2px}
.sw{position:relative;width:46px;height:26px;flex:none}
.sw input{position:absolute;top:0;left:0;width:100%;height:100%;margin:0;opacity:0;cursor:pointer;z-index:1}
.sw span{position:absolute;top:0;left:0;right:0;bottom:0;border:1px solid var(--fg);border-radius:var(--r);transition:.15s}
.sw span::after{content:"";position:absolute;top:4px;left:4px;width:16px;height:16px;border-radius:3px;background:var(--fg);transition:.15s}
.sw input:checked+span{background:var(--fg)}
.sw input:checked+span::after{left:24px;background:var(--bg)}
.sw input:focus-visible+span{outline:2px solid var(--mid);outline-offset:2px}
dialog{background:var(--bg);color:var(--fg);border:2px solid var(--fg);border-radius:8px;padding:20px;width:min(420px,92vw);font-family:inherit}
dialog::backdrop{background:rgba(0,0,0,.6)}
dialog h3{margin:0 0 14px;font-size:16px}
#xdlg{width:min(680px,94vw)}
.warn{font-size:12px;margin-top:6px;padding-left:8px;border-left:3px solid var(--fg)}
.warn:empty{display:none}
#xdlg textarea{display:block;width:100%;height:260px;box-sizing:border-box;padding:10px;font-family:ui-monospace,Menlo,Consolas,monospace;font-size:12px;resize:vertical;border:1px solid var(--fg);border-radius:var(--r);background:var(--bg);color:var(--fg)}
#xdlg p{margin:0 0 12px}
#xdlg .r{display:flex;gap:10px;justify-content:flex-end;margin-top:14px}
#gdlg{width:min(460px,94vw)}
#gout{display:flex;align-items:center;box-sizing:border-box;min-height:52px;font-family:ui-monospace,Menlo,Consolas,monospace;font-size:18px;line-height:1.3;word-break:break-all;border:1px solid var(--fg);border-radius:var(--r);padding:10px 12px;margin:0 0 16px;user-select:all}
.gopt{display:flex;align-items:center;gap:10px;margin:0 0 10px;font-size:14px}
.gopt input[type=range]{flex:1;accent-color:var(--fg)}
.gopt input[type=checkbox]{appearance:none;-webkit-appearance:none;flex:none;width:22px;height:22px;margin:0;border:2px solid var(--fg);border-radius:4px;background:var(--bg);display:grid;place-content:center;cursor:pointer}
.gopt input[type=checkbox]::after{content:"";width:5px;height:10px;border:solid var(--bg);border-width:0 3px 3px 0;transform:rotate(45deg) translate(-1px,-1px);opacity:0}
.gopt input[type=checkbox]:checked{background:var(--fg)}
.gopt input[type=checkbox]:checked::after{opacity:1}
.gopt input[type=checkbox]:focus-visible{outline:2px solid var(--mid);outline-offset:2px}
.gopt label{display:flex;align-items:center;gap:10px;cursor:pointer}
#glen{min-width:2ch;text-align:right;font-weight:700}
#gdlg .r{display:flex;gap:10px;justify-content:flex-end;margin-top:14px}
#xst{min-height:20px;margin:10px 0 0;font-size:13px}
#pdlg{display:none;width:100vw;height:100vh;height:var(--h,100vh);max-width:none;max-height:none;margin:0;border:0;border-radius:0;padding:0}
#pdlg[open]{display:flex;flex-direction:column}
.mhead{flex:none;display:flex;align-items:center;gap:12px;padding:12px 16px;border-bottom:2px solid var(--fg)}
#mpane{flex:1;overflow:auto;padding:16px}
.mfoot{flex:none;display:flex;justify-content:space-between;align-items:center;gap:8px;padding:12px 16px calc(12px + env(safe-area-inset-bottom));border-top:2px solid var(--fg)}
#mmsg{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
#gate{position:fixed;top:0;left:0;right:0;height:var(--h,100vh);z-index:50;background:var(--bg);display:flex;align-items:center;justify-content:center;text-align:center;padding:24px}
#gate[hidden]{display:none}
#gate h2{font-size:20px;margin-bottom:8px}
.logo.big{display:block;width:128px;height:128px;margin:0 auto 22px}
.dd{position:relative;width:200px;flex:none}
.dd-btn{display:flex;align-items:center;gap:10px;cursor:pointer;text-align:left;position:relative;padding-right:30px}
.dd-btn::after{content:"";position:absolute;right:12px;top:50%;margin-top:-2px;border:5px solid transparent;border-top-color:var(--fg);border-bottom:0}
.dd-list{position:absolute;right:0;top:44px;width:220px;max-height:min(50vh,340px);overflow:auto;background:var(--bg);border:1px solid var(--fg);border-radius:var(--r);padding:4px;z-index:10}
.dd-opt{display:flex;align-items:center;gap:10px;width:100%;height:36px;padding:0 8px;border:0;border-radius:4px;background:none;color:var(--fg);font-size:14px;cursor:pointer;text-align:left}
.dd-opt.on{background:var(--dim)}
.dd-opt:hover{background:var(--fg);color:var(--bg)}
.sw2{display:inline-block;flex:none;width:20px;height:20px;border-radius:5px;border:1px solid var(--mid)}
@media(max-width:760px){
  .dd{width:170px}
  .set .sel-wrap{width:150px}
  .app{padding:0 14px}
  h1{padding:12px 0 10px;font-size:18px;gap:8px}
  .logo{width:32px;height:32px}
  .frame{grid-template-columns:1fr}
  section{display:none}
  aside{border-right:0;padding-right:0}
  .btn{padding:0 11px}
  .btn,.in{height:42px}
  #bar{gap:8px}
}
</style>
</head>
<body>
<div class="app">
  <h1><span class="logo" role="img" aria-label="NoxPass logo"></span>NoxPass Bridge</h1>
  <div class="frame" id="frame">
    <aside>
      <div class="r">
        <input class="in" id="q" placeholder="Search..." aria-label="Search passwords" autocomplete="off">
        <button class="btn sq" data-act="new" title="New password" aria-label="New password">+</button>
      </div>
      <div class="count mid" id="count"></div>
      <div id="list"></div>
      <div id="setrow"></div>
    </aside>
    <section id="pane"></section>
  </div>
  <div class="rule" id="rule"><i></i></div>
  <div id="bar">
    <span id="msg"></span>
    <span id="btns">
      <button class="btn" id="bDiscard" data-act="discard">Discard</button>
      <button class="btn" id="bValidate" data-act="validate">Validate</button>
    </span>
  </div>
</div>

<div id="gate"><div><span class="logo big" role="img" aria-label="NoxPass logo"></span><h2 id="gt">Connecting…</h2><p class="mid" id="gs"></p></div></div>
<dialog id="dlg"></dialog>
<dialog id="xdlg">
  <h3 id="xt"></h3><p class="mid" id="xhint"></p>
  <textarea id="xta" spellcheck="false" autocomplete="off"></textarea>
  <div id="xst" class="mid"></div>
  <div class="r"><button class="btn" id="xclose">Close</button><button class="btn" id="xgo"></button></div>
</dialog>
<dialog id="gdlg">
  <h3>Password generator</h3>
  <div id="gout" aria-live="polite"></div>
  <div class="gopt"><span>Length</span><input type="range" id="gr" min="4" max="64" value="20" aria-label="Length"><span id="glen">20</span></div>
  <div class="gopt"><label><input type="checkbox" id="gd" checked>Digits (0-9)</label></div>
  <div class="gopt"><label><input type="checkbox" id="gs2" checked><span>Special characters</span></label></div>
  <div class="r"><button class="btn" id="gnew">Regenerate</button><button class="btn" id="gcancel">Cancel</button><button class="btn" id="guse">Use</button></div>
</dialog>
<dialog id="pdlg">
  <div class="mhead"><button class="btn" id="mclose">Back</button><b id="mtitle"></b></div>
  <div id="mpane"></div>
  <div class="mfoot"><span id="mmsg"></span><button class="btn" id="mValidate" data-act="validate">Validate</button></div>
</dialog>

<script>
/* N°, name, hidden on the stick, name stored (only Text and Secret have a name you choose) */
var TYPES=[
 {n:1,name:'Username / Email',obf:0,lab:0},{n:2,name:'Password',obf:1,lab:0},{n:3,name:'URL',obf:0,lab:0},
 {n:4,name:'Card number',obf:1,lab:0},{n:5,name:'Expiry',obf:0,lab:0},{n:6,name:'CVV',obf:1,lab:0},
 {n:7,name:'Note',obf:0,lab:0},{n:8,name:'Text',obf:0,lab:1},{n:9,name:'Secret',obf:1,lab:1}];
var T=function(n){return TYPES.filter(function(t){return t.n===n})[0]||TYPES[0]};
var LIM={title:32,label:24,value:128,fields:16,entries:500,presets:16};
var SETTINGS=[
 {k:'theme',name:'Theme',hint:'Colors of the stick screen',type:'theme'},
 {k:'layout',name:'USB keyboard',hint:'Layout of the computer you type on',type:'select',opts:[['azerty','AZERTY'],['qwerty','QWERTY']]},
 {k:'sort',name:'Sort',hint:'Order of the password list on the stick',type:'select',opts:[['recent','Recent'],['alpha','A-Z']]},
 {k:'pinDigits',name:'Show PIN digits',hint:'Off: validated digits become *',type:'switch'},
 {k:'brightness',name:'Brightness',hint:'Screen backlight of the stick',type:'range'},
 {k:'autoLock',name:'Auto-lock',hint:'Lock the vault after inactivity',type:'select',opts:[['0','Never'],['30','30 seconds'],['60','1 minute'],['300','5 minutes']]},
 {k:'pinShuffle',name:'Shuffle PIN pad',hint:'The dial starts on a random digit at every entry',type:'switch'},
 {k:'offOnUnplug',name:'Off on unplug',hint:'The stick turns itself off when the USB cable is removed',type:'switch'}];

/* The theme list comes from the stick (same table as its Settings screen); this is only a fallback until it loads. */
var THEMES=[{k:'dark',n:'Dark',bg:'#000000',fg:'#FFFFFF'},{k:'light',n:'Light',bg:'#FFFFFF',fg:'#000000'}];
var TH=function(k){return THEMES.filter(function(t){return t.k===k})[0]||THEMES[0]};
function mix(a,b,t){
  var pa=parseInt(a.slice(1),16),pb=parseInt(b.slice(1),16),o=0;
  for(var s=16;s>=0;s-=8){var x=(pa>>s)&255,y=(pb>>s)&255;o=(o<<8)|Math.round(x+(y-x)*t)}
  return '#'+('000000'+o.toString(16)).slice(-6);
}
function swatch(t){return '<i class="sw2" style="background:linear-gradient(135deg,'+t.bg+' 50%,'+t.fg+' 50%)"></i>'}
var ddOpen=false;
function themeDD(v){
  var t=TH(v);
  return '<div class="dd"><button class="in dd-btn" data-dd="1">'+swatch(t)+'<span>'+t.n+'</span></button>'+
    (ddOpen?'<div class="dd-list">'+THEMES.map(function(x){return '<button class="dd-opt'+(x.k===v?' on':'')+'" data-theme="'+x.k+'">'+swatch(x)+'<span>'+x.n+'</span></button>'}).join('')+'</div>':'')+'</div>';
}
/* The phone's address bar makes 100vh taller than what is visible: use the real visible height. */
function setH(){document.documentElement.style.setProperty('--h',(window.visualViewport?window.visualViewport.height:window.innerHeight)+'px')}
setH();addEventListener('resize',setH);addEventListener('orientationchange',setH);
if(window.visualViewport)window.visualViewport.addEventListener('resize',setH);

var clone=function(o){return JSON.parse(JSON.stringify(o))};
var esc=function(s){return String(s).replace(/[&<>"']/g,function(c){return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]})};
var $=function(s){return document.querySelector(s)};
var mq=matchMedia('(max-width:760px)');
var mobile=function(){return mq.matches};
var bl=function(s){return new TextEncoder().encode(s).length};

var saved={entries:[],presets:[],settings:{theme:'dark',layout:'qwerty',pinDigits:true,pinShuffle:false,offOnUnplug:false,sort:'recent',autoLock:60,brightness:100}};
var draft=clone(saved),sel=null,q='',uid=0,busy=false,loaded=false,flash='',flashT=0;
var shown={};
var cur=function(){return typeof sel==='number'?draft.entries.filter(function(e){return e.id===sel})[0]:null};

/* Special characters: only ones the stick can type (all are plain ASCII). */
var GSYM='!@#$%^&*()-_=+[]{};:,.?/';
var GL='abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ',GD='0123456789';
function rnd(n){ /* unbiased random integer in [0,n) */
  var m=Math.floor(4294967296/n)*n,a=new Uint32Array(1);
  do{crypto.getRandomValues(a)}while(a[0]>=m);return a[0]%n;
}
function gen(n,sets){
  var all=sets.join(''),c=[],i,j,t;
  for(i=0;i<sets.length&&i<n;i++)c.push(sets[i][rnd(sets[i].length)]);   /* at least one of each kind */
  while(c.length<n)c.push(all[rnd(all.length)]);
  for(i=c.length-1;i>0;i--){j=rnd(i+1);t=c[i];c[i]=c[j];c[j]=t}           /* shuffle */
  return c.join('');
}
var gTarget=-1;
function gSets(){var s=[GL];if($('#gd').checked)s.push(GD);if($('#gs2').checked)s.push(GSYM);return s}
function gRefresh(){
  var s=gSets();$('#glen').textContent=$('#gr').value;
  $('#gout').textContent=gen(+$('#gr').value,s);
}
function gOpen(i){
  gTarget=i;
  try{var p=JSON.parse(localStorage.getItem('gen')||'{}');
    if(p.n>=4&&p.n<=64)$('#gr').value=p.n;
    if(typeof p.d==='boolean')$('#gd').checked=p.d;if(typeof p.s==='boolean')$('#gs2').checked=p.s}catch(e){}
  gRefresh();$('#gdlg').showModal();
}
function gClose(){$('#gout').textContent='';$('#gdlg').close()}
function status(e){
  if(e.deleted)return 'deleted';
  var s=saved.entries.filter(function(x){return x.id===e.id})[0];
  if(!s)return 'new';
  return JSON.stringify(s)!==JSON.stringify(e)?'modified':'';
}
function fmt(d,v){if(d.type==='switch')return v?'On':'Off';if(d.type==='range')return v+'%';if(d.type==='theme')return TH(v).n;return d.opts.filter(function(o){return o[0]===String(v)})[0][1].split(' · ')[0]}
function presetsDiff(){return JSON.stringify(saved.presets)!==JSON.stringify(draft.presets)}
function settingsDiff(){return SETTINGS.filter(function(d){return String(saved.settings[d.k])!==String(draft.settings[d.k])})}
function pending(){
  var p=[];
  draft.entries.forEach(function(e){var s=status(e);if(s)p.push({s:s,t:e.title||'(untitled)'})});
  settingsDiff().forEach(function(d){p.push({s:'setting',t:d.name+': '+fmt(d,saved.settings[d.k])+' to '+fmt(d,draft.settings[d.k])})});
  if(presetsDiff())p.push({s:'setting',t:'Username/Email presets: '+saved.presets.length+' to '+draft.presets.length});
  return p;
}
/* Only plain ASCII (printable characters) is allowed: the stick types on a USB keyboard and cannot type anything else. */
function nonAscii(v){var o=[],i;for(i=0;i<v.length;i++){var c=v.charAt(i),n=v.charCodeAt(i);if((n<32&&n!==10||n>126)&&o.indexOf(c)<0)o.push(c)}return o}
function atxt(v){var u=nonAscii(v);return u.length?'Not allowed: '+u.join(' ')+'. Use plain characters only (letters, digits, standard symbols).':''}
function problems(){
  if(draft.presets.length>LIM.presets)return 'Too many presets (max '+LIM.presets+')';
  for(var j=0;j<draft.presets.length;j++){var pr=draft.presets[j];if(bl(pr)>LIM.value)return 'A username/email preset is too long: '+pr.slice(0,20);if(nonAscii(pr).length)return 'Username/email preset '+pr.slice(0,20)+': plain characters only'}
  for(var i=0;i<draft.entries.length;i++){
    var e=draft.entries[i];if(e.deleted||!status(e))continue;
    var nm=e.title||'(untitled)';
    if(!e.title.trim())return 'An entry has no title';
    if(nonAscii(e.title).length)return nm+': the title has characters that are not allowed';
    if(bl(e.title)>LIM.title)return nm+': title too long';
    if(!e.fields.length)return nm+': no fields';
    if(e.fields.length>LIM.fields)return nm+': too many fields';
    for(var k=0;k<e.fields.length;k++){
      var f=e.fields[k],t=T(f.t);
      if(t.lab&&!f.l.trim())return nm+': a '+t.name+' field needs a name';
      if(t.lab&&bl(f.l)>LIM.label)return nm+': a name is too long';
      if(bl(f.v)>LIM.value)return nm+': a value is too long';
      if(nonAscii(f.v).length||nonAscii(f.l).length)return nm+': plain characters only (no accents or special symbols)';
    }
  }
  return '';
}

/* ---------- rendering ---------- */
function renderList(){
  var items=draft.entries.filter(function(e){return e.title.toLowerCase().indexOf(q.toLowerCase())>=0});
  $('#list').innerHTML=items.map(function(e){
    var s=status(e);
    return '<div class="item '+(e.id===sel?'sel':'')+' '+(e.deleted?'del':'')+'" data-id="'+e.id+'"><span>'+esc(e.title||'(untitled)')+'</span>'+(s?'<span class="badge">'+s+'</span>':'')+'</div>';
  }).join('')||(loaded?(q?'<p class="hint">No password matches "'+esc(q)+'".</p>':'<p class="hint">No password yet. Press + to add one.</p>'):'');
  $('#count').textContent=loaded?(q?items.length+' of '+draft.entries.length:draft.entries.length)+(draft.entries.length===1?' password':' passwords'):'';
  $('#setrow').innerHTML='<div class="item '+(sel==='settings'?'sel':'')+'" data-id="settings"><span>Settings</span>'+(settingsDiff().length?'<span class="badge">modified</span>':'')+'</div><div class="item '+(sel==='presets'?'sel':'')+'" data-id="presets" style="margin-top:6px"><span>Username/Email presets</span>'+(presetsDiff()?'<span class="badge">modified</span>':'')+'</div>';
}
function typeSelect(name,value,dis,cls){
  return '<div class="sel-wrap '+cls+'"><select class="in" data-k="'+name+'" '+dis+'>'+TYPES.map(function(t){return '<option value="'+t.n+'"'+(t.n===value?' selected':'')+'>'+t.name+'</option>'}).join('')+'</select></div>';
}
function renderPane(){
  var box=mobile()?$('#mpane'):$('#pane'),e=cur();
  if(mobile()&&sel!=='settings'&&sel!=='presets'&&!e&&$('#pdlg').open)$('#pdlg').close();
  if(sel==='settings'){
    $('#mtitle').textContent='Settings';
    box.innerHTML='<h2>Settings</h2>'+SETTINGS.map(function(d){
      var v=draft.settings[d.k],ctl=d.type==='theme'?themeDD(v):d.type==='range'
        ?'<div class="rng"><input type="range" min="0" max="100" step="10" data-s="'+d.k+'" value="'+v+'"><output>'+v+'%</output></div>'
        :d.type==='switch'
        ?'<label class="sw"><input type="checkbox" data-s="'+d.k+'" '+(v?'checked':'')+'><span></span></label>'
        :'<div class="sel-wrap"><select class="in" data-s="'+d.k+'">'+d.opts.map(function(o){return '<option value="'+o[0]+'"'+(o[0]===String(v)?' selected':'')+'>'+o[1]+'</option>'}).join('')+'</select></div>';
      return '<div class="set"><div><b>'+d.name+'</b><small>'+d.hint+'</small></div>'+ctl+'</div>';
    }).join('')+'<div class="set"><div><b>Export</b><small>Shows every password as plain JSON text, to copy. Nothing is saved on this device.</small></div><button class="btn" data-act="export">Export</button></div>'
      +'<div class="set" style="border-bottom:0"><div><b>Import</b><small>Paste a JSON export. The entries are added to your vault.</small></div><button class="btn" data-act="import">Import</button></div>'
      +'<p class="hint">To change the PIN or reset the stick, use Settings on the stick itself.</p>';
    return;
  }
  if(sel==='presets'){
    $('#mtitle').textContent='Username/Email presets';
    box.innerHTML='<h2>Username/Email presets</h2><div class="set" style="border-bottom:0"><div><b>Saved usernames and emails</b><small>One per line, up to '+LIM.presets+'. On the stick, they are offered first when you type a username or email.</small></div></div>'
      +'<textarea class="in" data-pre="1" rows="10" spellcheck="false" autocomplete="off" style="width:100%;height:260px;padding:10px 12px;line-height:1.5;resize:none;font-family:inherit">'+esc(draft.presets.join('\n'))+'</textarea>'
      +'<div class="warn" id="prew"></div>';
    return;
  }
  if(!e){box.innerHTML=mobile()?'':'<p class="hint">Select a password on the left to see or edit it, or press + to create one.</p>';return}
  $('#mtitle').textContent='Password';
  if(!e.fields){        /* the stick only sent the title: its content is fetched when the entry is opened */
    box.innerHTML='<input class="in title" value="'+esc(e.title)+'" disabled>'+
      '<div class="set"><small>'+(fetching[e.id]?'Asking the stick…':'Could not load this entry. Click it again to retry.')+'</small></div>'+
      '<div class="actions">'+(e.deleted?'<button class="btn" data-act="restore">Restore</button>':'<button class="btn" data-act="del">Delete</button>')+'</div>';
    return;
  }
  var d=e.deleted?'disabled':'';
  box.innerHTML='<input class="in title" data-k="title" placeholder="Title" maxlength="'+LIM.title+'" value="'+esc(e.title)+'" '+d+'>'+
    e.fields.map(function(f,i){
      var t=T(f.t),vis=!t.obf||shown[i];
      return '<div class="field"><div class="r">'+typeSelect('type:'+i,f.t,d,'t'+(t.lab?'':' full'))+
        (t.lab?'<input class="in" data-k="label:'+i+'" placeholder="Name" maxlength="'+LIM.label+'" value="'+esc(f.l)+'" '+d+'>':'')+'</div>'+
        '<div class="r" style="margin-top:8px"><input class="in" data-k="value:'+i+'" type="'+(vis?'text':'password')+'" placeholder="Value" maxlength="'+LIM.value+'" value="'+esc(f.v)+'" '+d+' spellcheck="false" autocomplete="off">'+
        (t.obf?'<button class="btn" data-act="reveal" data-i="'+i+'">'+(vis?'Hide':'Show')+'</button>':'')+
        (f.t===2?'<button class="btn" data-act="gen" data-i="'+i+'" '+d+'>Generate</button>':'')+
        '<button class="btn" data-act="rmfield" data-i="'+i+'" '+d+' title="Remove field">✕</button></div><div class="warn" data-w="'+i+'">'+atxt(f.v)+'</div></div>';
    }).join('')+
    '<div class="actions"><button class="btn" data-act="addfield" '+((d||e.fields.length>=LIM.fields)?'disabled':'')+'>+ Field</button>'+
    (e.deleted?'<button class="btn" data-act="restore">Restore</button>':'<button class="btn" data-act="del">Delete</button>')+'</div>';
}
function paint(bg,fg){
  var st=document.documentElement.style;
  st.setProperty('--bg',bg);st.setProperty('--fg',fg);
  st.setProperty('--mid',mix(bg,fg,.55));st.setProperty('--dim',mix(bg,fg,.16));
}
function renderBar(){
  var th=TH(draft.settings.theme);paint(th.bg,th.fg);                         // live preview of the theme
  var n=pending().length,txt=flash||(n?n+' pending change'+(n>1?'s':''):'No pending changes');
  $('#msg').textContent=txt;$('#mmsg').textContent=txt;
  $('#bDiscard').disabled=$('#bValidate').disabled=$('#mValidate').disabled=!n||busy;
}
function renderAll(){renderList();renderPane();renderBar()}
function openModal(){if(mobile()&&sel!==null&&!$('#pdlg').open)$('#pdlg').showModal()}
function say(text,ms){
  flash=text;clearTimeout(flashT);
  if(ms)flashT=setTimeout(function(){flash='';renderBar()},ms);
  renderBar();
}
function gate(title,sub){
  var g=$('#gate');
  if(!title){g.hidden=true;return}
  g.hidden=false;$('#gt').textContent=title;$('#gs').textContent=sub||'';
}

/* ---------- talking to the stick ---------- */
function api(path,body){
  var o={cache:'no-store',headers:{'X-Bridge':'1'}};
  if(body){o.method='POST';o.headers['Content-Type']='application/json';o.body=JSON.stringify(body)}
  return fetch(path,o).then(function(r){return r.json()});
}
function load(){
  return api('/api/vault').then(function(j){
    if(j.error)throw new Error(j.error);
    LIM=j.limits;if(j.themes&&j.themes.length)THEMES=j.themes;
    saved={entries:j.entries.map(function(e){return {id:e.id,title:e.title}}),presets:j.presets||[],settings:j.settings};   /* titles only */
    draft=clone(saved);sel=null;shown={};loaded=true;
    renderAll();
  });
}
var fetching={};
/* Ask the stick for ONE entry's content, only when it is opened. */
function ensure(id){
  var e=cur();if(!e||e.fields||id<0||fetching[id])return;
  fetching[id]=1;
  api('/api/entry?id='+id).then(function(j){
    if(j.error)throw new Error(j.error);
    var fl=j.fields.map(function(f){return {t:f.t,l:f.l||'',v:f.v}});
    [saved,draft].forEach(function(st){var x=st.entries.filter(function(z){return z.id===id})[0];if(x&&!x.fields)x.fields=clone(fl)});
  }).catch(function(er){say('Could not load the entry: '+(er.message||'error'),4000)})
  .then(function(){delete fetching[id];renderList();renderPane();renderBar()});
}
function tick(){
  var next=1500;
  if(busy){setTimeout(tick,1000);return}
  api('/api/status').then(function(s){
    if(!loaded&&s.bg&&s.fg)paint(s.bg,s.fg);                                   // waiting page: same colors as the stick
    if(s.me&&s.s==='connected'){
      next=3000;
      if(!loaded)return load().then(function(){gate(null)});
      gate(null);
    }else if(s.me&&s.s==='pending'){loaded=false;gate('Waiting for approval','Press A on the M5Stick to accept this device.')}
    else if(s.s==='waiting'){loaded=false;gate('Disconnected','This device is no longer connected to the M5Stick.')}
    else{loaded=false;gate('Busy','Another device is connected to the M5Stick.')}
  }).catch(function(){gate('Cannot reach the M5Stick','Check that you are still on its Wi-Fi.')})
  .then(function(){setTimeout(tick,next)});
}
function buildBody(){
  var ops=[];
  draft.entries.forEach(function(e){
    var s=status(e);if(!s)return;
    if(s==='deleted'){ops.push({op:'del',id:e.id});return}
    var fields=e.fields.map(function(f){var o={t:f.t,v:f.v};if(T(f.t).lab)o.l=f.l;return o});
    ops.push(s==='new'?{op:'add',title:e.title,fields:fields}:{op:'upd',id:e.id,title:e.title,fields:fields});
  });
  var body={ops:ops};
  if(settingsDiff().length)body.settings=draft.settings;
  if(presetsDiff())body.presets=draft.presets;
  return body;
}
function send(){
  var n=pending().length;busy=true;
  $('#rule').classList.add('busy');$('#frame').classList.add('busy-lock');
  $('#rule i').style.width='35%';
  say('Sending '+n+' change'+(n>1?'s':'')+'…',0);
  api('/api/apply',buildBody()).then(function(j){
    $('#rule i').style.width='100%';
    if(!j.ok)throw new Error(j.error||'Rejected by the stick');
    return load().then(function(){say('Applied on the stick ('+j.n+' change'+(j.n>1?'s':'')+')',3000)});
  }).catch(function(e){say('Failed: '+(e.message||'error'),5000)})
  .then(function(){
    setTimeout(function(){$('#rule').classList.remove('busy');$('#rule i').style.width='0'},600);
    busy=false;$('#frame').classList.remove('busy-lock');renderAll();
  });
}

/* ---------- actions ---------- */
document.addEventListener('click',function(ev){
  var t=ev.target;
  if(!busy){
    var o=t.closest('[data-theme]');
    if(o){draft.settings.theme=o.dataset.theme;ddOpen=false;renderPane();renderList();renderBar();return}
    if(t.closest('[data-dd]')){ddOpen=!ddOpen;renderPane();return}
    if(ddOpen&&!t.closest('.dd')){ddOpen=false;renderPane()}
  }
  var b=t.closest('[data-act],[data-id]');if(!b||busy||b.closest('#dlg')||b.closest('#xdlg')||b.closest('#gdlg'))return;
  if(b.dataset.id){sel=(b.dataset.id==='settings'||b.dataset.id==='presets')?b.dataset.id:+b.dataset.id;shown={};ddOpen=false;renderList();ensure(sel);renderPane();return openModal()}
  var i=+b.dataset.i,e=cur();
  switch(b.dataset.act){
    case 'export':return startExport();
    case 'import':return startImport();
    case 'new':
      if(draft.entries.length>=LIM.entries)return say('Vault is full',3000);
      var n={id:--uid,title:'',fields:[{t:1,l:'',v:''},{t:2,l:'',v:gen(20,[GL,GD,GSYM])}]};
      draft.entries.unshift(n);sel=n.id;q='';$('#q').value='';shown={};
      renderAll();openModal();$('[data-k=title]').focus();break;
    case 'del':
      if(!saved.entries.some(function(x){return x.id===e.id})){draft.entries=draft.entries.filter(function(x){return x!==e});sel=null}
      else e.deleted=true;
      renderAll();break;
    case 'restore':delete e.deleted;renderAll();break;
    case 'addfield':e.fields.push({t:8,l:'',v:''});renderAll();break;
    case 'rmfield':e.fields.splice(i,1);shown={};renderAll();break;
    case 'reveal':shown[i]=!shown[i];renderPane();break;
    case 'gen':gOpen(i);break;
    case 'discard':
      draft=clone(saved);if(typeof sel==='number'&&!cur())sel=null;shown={};renderAll();say('Changes discarded',3000);break;
    case 'validate':
      var pr=problems();if(pr)return say(pr,5000);
      $('#dlg').innerHTML='<h3>Send to the M5Stick?</h3>'+pending().map(function(x){return '<div class="item static"><span>'+esc(x.t)+'</span><span class="badge">'+x.s+'</span></div>'}).join('')+
        ''+'<p class="mid" style="margin:4px 0 16px">Only these changes are sent, not the whole vault.</p><div class="r" style="justify-content:flex-end"><button class="btn" data-act="cancel">Cancel</button><button class="btn" data-act="send">Send</button></div>';
      $('#dlg').showModal();break;
  }
});
$('#dlg').addEventListener('click',function(ev){
  var b=ev.target.closest('[data-act]');if(!b)return;
  $('#dlg').close();if(b.dataset.act==='send')send();
});
$('#mclose').addEventListener('click',function(){$('#pdlg').close()});
$('#pdlg').addEventListener('close',function(){sel=null;shown={};renderList()});
mq.addEventListener('change',function(){if($('#pdlg').open)$('#pdlg').close();renderAll()});

document.addEventListener('input',function(ev){
  var t=ev.target;
  if(t.id==='q'){q=t.value;return renderList()}
  if(t.dataset.pre){
    var seen={};draft.presets=t.value.split('\n').map(function(x){return x.trim()}).filter(function(x){if(!x||seen[x])return false;seen[x]=1;return true});
    $('#prew').textContent=problems();renderList();return renderBar()}
  if(t.dataset.s){draft.settings[t.dataset.s]=t.type==='checkbox'?t.checked:(t.dataset.s==='autoLock'||t.dataset.s==='brightness'?+t.value:t.value);if(t.type==='range'&&t.nextSibling)t.nextSibling.textContent=t.value+'%';renderList();return renderBar()}
  var k=t.dataset.k,e=cur();if(!k||!e)return;
  var p=k.split(':'),name=p[0],i=+p[1];
  if(name==='title')e.title=t.value;
  else if(name==='label')e.fields[i].l=t.value;
  else if(name==='value'){e.fields[i].v=t.value;var w=document.querySelector('[data-w="'+i+'"]');if(w)w.textContent=atxt(t.value)}
  else if(name==='type'){var f=e.fields[i];f.t=+t.value;if(!T(f.t).lab)f.l='';shown[i]=false;renderPane()}
  renderList();renderBar();
});

/* ---------- export / import (JSON) ---------- */
var TK=['','username','password','url','card','expiry','cvv','note','text','secret'];
var asc=function(v){return JSON.stringify(v).replace(/[\u007f-￿]/g,function(c){return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4)})};
function entryLine(e,last){
  return '{"title":'+asc(e.title)+',"fields":['+e.fields.map(function(f){
    return '{"type":"'+(TK[f.t]||'text')+'"'+(T(f.t).lab?',"label":'+asc(f.l||''):'')+',"value":'+asc(f.v)+'}'}).join(',')+']}'+(last?'':',')+'\n';
}
var xBusy=false;
function xopen(title,hint,go){
  $('#xt').textContent=title;$('#xhint').textContent=hint;$('#xst').textContent='';$('#xta').value='';
  $('#xgo').textContent=go;$('#xgo').hidden=!go;$('#xgo').disabled=false;$('#xta').readOnly=false;
  $('#xdlg').showModal();
}
function xclose(){if(xBusy)return;$('#xta').value='';$('#xdlg').close()}
function startExport(){
  var ids=saved.entries.map(function(e){return e.id}),out='[\n',i=0;
  xopen('Export','Clear text. Nothing is saved on this device: copy it somewhere safe. Closing this window erases it.','Copy');
  $('#xta').readOnly=true;$('#xgo').disabled=true;xBusy=true;busy=true;
  function next(){
    if(i>=ids.length){
      $('#xta').value=out+']';$('#xst').textContent=ids.length+' entries ready.';
      $('#xgo').disabled=false;xBusy=false;busy=false;return;
    }
    $('#xst').textContent='Asking the stick… '+(i+1)+' / '+ids.length;
    api('/api/entry?id='+ids[i]).then(function(j){
      if(j.error)throw new Error(j.error);
      out+=entryLine({title:j.title,fields:j.fields.map(function(f){return {t:f.t,l:f.l||'',v:f.v}})},i===ids.length-1);
      i++;next();
    }).catch(function(er){$('#xst').textContent='Failed: '+(er.message||'error')+'. The export is incomplete.';xBusy=false;busy=false});
  }
  next();
}
function parseImport(text){
  var r={ok:[],bad:[],err:''},o;
  try{o=JSON.parse(text)}catch(er){r.err='Not valid JSON: '+er.message;return r}
  var list=Array.isArray(o)?o:(o&&Array.isArray(o.entries)?o.entries:null);
  if(!list){r.err='No "entries" list found';return r}
  list.forEach(function(x,n){
    var why='',t=x&&typeof x.title==='string'?x.title:'',fl=x&&Array.isArray(x.fields)?x.fields:null,fs=[];
    if(!fl)why='no fields';
    else if(bl(t)>LIM.title)why='title too long';
    else if(nonAscii(t).length)why='title has characters that are not allowed';
    else if(fl.length>LIM.fields)why='too many fields';
    else for(var k=0;k<fl.length&&!why;k++){
      var f=fl[k]||{},ty=typeof f.type==='string'?TK.indexOf(f.type):(+f.t||0),v=typeof f.value==='string'?f.value:(typeof f.v==='string'?f.v:''),
          l=typeof f.label==='string'?f.label:(typeof f.l==='string'?f.l:'');
      if(ty<1||ty>9)why='unknown field type';
      else if(bl(v)>LIM.value)why='value too long';
      else if(nonAscii(v).length||nonAscii(l).length)why='characters that are not allowed (plain ASCII only)';
      else if(T(ty).lab&&bl(l)>LIM.label)why='label too long';
      else fs.push({t:ty,l:T(ty).lab?l:'',v:v});
    }
    if(why)r.bad.push('#'+(n+1)+' '+(t||'(untitled)')+': '+why);else r.ok.push({title:t,fields:fs});
  });
  return r;
}
var imp=null;
function checkImport(){
  var txt=$('#xta').value.trim();imp=null;$('#xgo').disabled=true;
  if(!txt){$('#xst').textContent='';return}
  var r=parseImport(txt);
  if(r.err){$('#xst').textContent=r.err;return}
  var room=LIM.entries-saved.entries.length,m=r.ok.length+' entries ready'+(r.bad.length?', '+r.bad.length+' skipped ('+r.bad.slice(0,2).join('; ')+(r.bad.length>2?'…':'')+')':'')+'.';
  if(!r.ok.length){$('#xst').textContent='Nothing to import. '+m;return}
  if(r.ok.length>room){$('#xst').textContent=m+' The vault only has room for '+room+' more.';return}
  imp=r.ok;$('#xst').textContent=m;$('#xgo').disabled=false;
}
function startImport(){
  xopen('Import','Paste the JSON here. Entries are added at the top of the list, in the same order.','Import');
  $('#xgo').disabled=true;imp=null;
}
function runImport(){
  var list=imp.slice().reverse(),total=list.length,done=0;           /* the stick puts each new entry first: send the last one first */
  xBusy=true;busy=true;$('#xgo').disabled=true;$('#xta').readOnly=true;
  function chunk(){
    if(!list.length)return Promise.resolve();
    var ops=[],size=0;
    while(list.length&&ops.length<20&&size<18000){
      var e=list.shift(),o={op:'add',title:e.title,fields:e.fields.map(function(f){var z={t:f.t,v:f.v};if(T(f.t).lab)z.l=f.l;return z})};
      size+=JSON.stringify(o).length;ops.push(o);
    }
    $('#xst').textContent='Importing… '+done+' / '+total;
    return api('/api/apply',{ops:ops}).then(function(j){
      if(!j.ok)throw new Error(j.error||'Rejected by the stick');
      done+=ops.length;return chunk();
    });
  }
  chunk().then(function(){
    $('#xst').textContent=done+' entries imported.';
  }).catch(function(er){
    $('#xst').textContent='Failed: '+(er.message||'error')+'. '+done+' of '+total+' imported, the rest was not changed.';
  }).then(function(){
    xBusy=false;busy=false;imp=null;$('#xta').value='';
    return load().then(function(){renderAll()}).catch(function(){});
  });
}
$('#xclose').addEventListener('click',xclose);
['gr','gd','gs2'].forEach(function(id){$('#'+id).addEventListener('input',gRefresh)});
$('#gnew').addEventListener('click',gRefresh);
$('#gcancel').addEventListener('click',gClose);
$('#guse').addEventListener('click',function(){
  var e=cur(),v=$('#gout').textContent;
  try{localStorage.setItem('gen',JSON.stringify({n:+$('#gr').value,d:$('#gd').checked,s:$('#gs2').checked}))}catch(x){}
  if(e&&e.fields[gTarget]){e.fields[gTarget].v=v;shown[gTarget]=true}
  gClose();renderAll();
});
$('#xdlg').addEventListener('cancel',function(ev){if(xBusy)ev.preventDefault()});
$('#xta').addEventListener('input',function(){if($('#xt').textContent==='Import')checkImport()});
$('#xgo').addEventListener('click',function(){
  if($('#xt').textContent==='Import'){if(imp)runImport();return}
  var ta=$('#xta');ta.select();var ok=false;try{ok=document.execCommand('copy')}catch(e){}
  $('#xst').textContent=ok?'Copied.':'Selected: press Ctrl+C (or long-press, then Copy).';
});

renderAll();
tick();
</script>
</body>
</html>
)HTML";
