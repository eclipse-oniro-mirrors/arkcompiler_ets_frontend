/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import type { GraphViewModel } from './viewModel';

/** Colors used to fill file nodes, one per {@link FileKind}. */
export interface GraphColors {
  static: string;
  dynamic: string;
}

/** Options controlling the generated HTML page. */
export interface RenderHtmlOptions {
  /** Page title and header text. */
  title?: string;
  /** Initial fill colors; users can still change them from the UI. */
  colors?: Partial<GraphColors>;
}

const DEFAULT_COLORS: GraphColors = {
  static: '#f4c430',
  dynamic: '#3b82f6',
};

/** Escape a JSON string so it can be safely embedded inside a `<script>` tag. */
function embedJson(value: unknown): string {
  return JSON.stringify(value)
    .replace(/</g, '\\u003c')
    .replace(/\u2028/g, '\\u2028')
    .replace(/\u2029/g, '\\u2029');
}

/**
 * Render a dependency graph as a self-contained, interactive HTML page.
 *
 * The output has no external dependencies: layout, zooming, tooltips and the
 * color pickers are all implemented with inline SVG and vanilla JavaScript, so
 * the file can be opened directly in any browser or shipped as a build artifact.
 */
export function renderGraphHtml(model: GraphViewModel, options: RenderHtmlOptions = {}): string {
  const title = options.title ?? 'Dependency Graph';
  const colors: GraphColors = { ...DEFAULT_COLORS, ...options.colors };
  const config = { title, colors };

  return [
    '<!DOCTYPE html>',
    '<html lang="en">',
    '<head>',
    '<meta charset="utf-8" />',
    '<meta name="viewport" content="width=device-width, initial-scale=1" />',
    '<title>' + escapeHtml(title) + '</title>',
    '<style>' + STYLE + '</style>',
    '</head>',
    '<body>',
    BODY,
    '<script>window.__GRAPH__=' + embedJson(model) + ';window.__CONFIG__=' + embedJson(config) + ';</script>',
    '<script>' + CLIENT_SCRIPT + '</script>',
    '</body>',
    '</html>',
    '',
  ].join('\n');
}

function escapeHtml(value: string): string {
  return value.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

const STYLE = [
  ':root{--bg:#0f172a;--panel:#111c34;--panel-2:#0b1526;--stroke:#22304d;--text:#e5edff;--muted:#93a4c4;--accent:#6366f1;}',
  '*{box-sizing:border-box;}',
  'html,body{margin:0;height:100%;font-family:"Segoe UI",Roboto,Helvetica,Arial,sans-serif;background:var(--bg);color:var(--text);}',
  '#app{display:flex;flex-direction:column;height:100vh;}',
  'header{display:flex;align-items:center;gap:16px;padding:12px 20px;background:linear-gradient(180deg,var(--panel),var(--panel-2));border-bottom:1px solid var(--stroke);}',
  'header h1{font-size:16px;font-weight:600;margin:0;letter-spacing:.3px;}',
  'header .stats{color:var(--muted);font-size:12.5px;}',
  '.spacer{flex:1;}',
  '.toolbar{display:flex;align-items:center;gap:8px;}',
  '.legend{display:flex;align-items:center;gap:14px;flex-wrap:wrap;}',
  '.legend .item{display:flex;align-items:center;gap:7px;font-size:12.5px;color:var(--muted);}',
  '.legend input[type=color]{width:22px;height:22px;padding:0;border:1px solid var(--stroke);border-radius:6px;background:transparent;cursor:pointer;}',
  '.legend .swatch{width:14px;height:14px;border-radius:50%;border:1px solid rgba(255,255,255,.35);}',
  'button.tool{display:inline-flex;align-items:center;justify-content:center;min-width:34px;height:32px;padding:0 10px;border:1px solid var(--stroke);background:var(--panel);color:var(--text);border-radius:8px;font-size:14px;cursor:pointer;transition:background .15s,border-color .15s;}',
  'button.tool:hover{background:#17233f;border-color:var(--accent);}',
  '#stage{position:relative;flex:1;overflow:hidden;background:radial-gradient(circle at 20% 15%,#16233f 0,var(--bg) 60%);}',
  '#svg{width:100%;height:100%;display:block;cursor:grab;}',
  '#svg.panning{cursor:grabbing;}',
  '.module-box{fill:rgba(255,255,255,.03);stroke:var(--stroke);stroke-width:1.5;rx:14;}',
  '.module-label{fill:var(--muted);font-size:13px;font-weight:600;}',
  '.edge{stroke:#3a4b70;stroke-width:1.2;fill:none;opacity:.55;}',
  '.node .node-shape{stroke:rgba(9,14,26,.85);stroke-width:1.5;cursor:pointer;transition:stroke .12s,stroke-width .12s;}',
  '.node text{fill:var(--text);font-size:11px;pointer-events:none;}',
  '.dim{opacity:.12;transition:opacity .12s;}',
  '.sdk-hidden{display:none;}',
  '.edge.hot{stroke:var(--accent);stroke-width:2.2;opacity:1;}',
  '.node.hot .node-shape{stroke:#fff;stroke-width:2.5;}',
  '#tooltip{position:absolute;pointer-events:none;z-index:10;max-width:420px;padding:10px 12px;background:rgba(9,14,26,.96);border:1px solid var(--stroke);border-radius:10px;box-shadow:0 12px 30px rgba(0,0,0,.45);font-size:12.5px;line-height:1.5;opacity:0;transform:translateY(4px);transition:opacity .12s,transform .12s;}',
  '#tooltip.show{opacity:1;transform:translateY(0);}',
  '#tooltip .name{font-weight:700;margin-bottom:4px;word-break:break-all;}',
  '#tooltip .row{color:var(--muted);}',
  '#tooltip .row b{color:var(--text);font-weight:600;}',
  '#tooltip .tag{display:inline-block;padding:1px 7px;border-radius:999px;font-size:11px;font-weight:600;color:#0b1120;}',
  '#empty{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:var(--muted);font-size:14px;}',
].join('');

const BODY = [
  '<div id="app">',
  '<header>',
  '<h1 id="title">Dependency Graph</h1>',
  '<span class="stats" id="stats"></span>',
  '<div class="spacer"></div>',
  '<div class="legend">',
  '<span class="item"><input type="color" id="color-static" /><span class="swatch" id="sw-static"></span>Static</span>',
  '<span class="item"><input type="color" id="color-dynamic" /><span class="swatch" id="sw-dynamic"></span>Dynamic</span>',
  '</div>',
  '<div class="toolbar">',
  '<button class="tool" id="zoom-in" title="Zoom in">+</button>',
  '<button class="tool" id="zoom-out" title="Zoom out">&minus;</button>',
  '<button class="tool" id="fit" title="Fit to screen">Fit</button>',
  '<button class="tool" id="toggle-sdk" title="Hide SDK files">Hide SDK</button>',
  '</div>',
  '</header>',
  '<div id="stage">',
  '<svg id="svg" xmlns="http://www.w3.org/2000/svg"></svg>',
  '<div id="tooltip"></div>',
  '<div id="empty" style="display:none">No files to display.</div>',
  '</div>',
  '</div>',
].join('');

/*
 * Client runtime. Written with plain string concatenation and the DOM SVG API
 * (no template literals, no "${...}") so it embeds safely inside this module and
 * ships with zero external dependencies.
 */
const CLIENT_SCRIPT = [
  '(function(){',
  '"use strict";',
  'var SVGNS="http://www.w3.org/2000/svg";',
  'var DATA=window.__GRAPH__||{modules:[],files:[]};',
  'var CONFIG=window.__CONFIG__||{title:"Dependency Graph",colors:{}};',
  'var colors=Object.assign({static:"#f4c430",dynamic:"#3b82f6"},CONFIG.colors||{});',
  'var CELL=96,R=21,PAD=30,HEADER=30,MOD_GAP=44,ROW_MAX=1600;',
  '',
  'function el(name,attrs){var e=document.createElementNS(SVGNS,name);if(attrs){for(var k in attrs){e.setAttribute(k,attrs[k]);}}return e;}',
  'function byId(id){return document.getElementById(id);}',
  '',
  '// index files and build layout',
  'var fileById={};DATA.files.forEach(function(f){fileById[f.id]=f;});',
  'var groups={};var order=[];',
  'DATA.modules.forEach(function(m){groups[m.id]={module:m,files:[]};order.push(m.id);});',
  'DATA.files.forEach(function(f){if(!groups[f.moduleId]){groups[f.moduleId]={module:{id:f.moduleId,label:f.moduleId},files:[]};order.push(f.moduleId);}groups[f.moduleId].files.push(f);});',
  'function isSdkGroup(mid){var files=groups[mid].files;return files.length>0&&files.every(function(f){return f.isSdk;});}',
  'order.sort(function(a,b){return Number(isSdkGroup(a))-Number(isSdkGroup(b));});',
  '',
  'var pos={};var moduleBoxes=[];var boxById={};',
  'order.forEach(function(mid){',
  '  var g=groups[mid];if(g.files.length===0){return;}',
  '  var box={id:mid,x:0,y:0,w:0,h:0,label:g.module.label,language:g.module.language,visible:true};',
  '  moduleBoxes.push(box);boxById[mid]=box;',
  '});',
  'function layout(hideSdk){',
  '  pos={};var cursorX=MOD_GAP,cursorY=MOD_GAP,rowH=0;',
  '  order.forEach(function(mid){',
  '    var box=boxById[mid];if(!box){return;}',
  '    var files=groups[mid].files.filter(function(f){return !hideSdk||!f.isSdk;});var n=files.length;',
  '    box.visible=n>0;if(n===0){return;}',
  '    var cols=Math.max(1,Math.ceil(Math.sqrt(n)));var rows=Math.ceil(n/cols);',
  '    var boxW=cols*CELL+PAD*2;var boxH=rows*CELL+PAD*2+HEADER;',
  '    if(cursorX>MOD_GAP&&cursorX+boxW>ROW_MAX){cursorX=MOD_GAP;cursorY+=rowH+MOD_GAP;rowH=0;}',
  '    box.x=cursorX;box.y=cursorY;box.w=boxW;box.h=boxH;',
  '    files.forEach(function(f,i){var c=i%cols,r=Math.floor(i/cols);pos[f.id]={x:box.x+PAD+c*CELL+CELL/2,y:box.y+HEADER+PAD+r*CELL+CELL/2};});',
  '    cursorX+=boxW+MOD_GAP;rowH=Math.max(rowH,boxH);',
  '  });',
  '}',
  'layout(false);',
  '',
  '// dependents count',
  'var dependents={};DATA.files.forEach(function(f){f.dependencies.forEach(function(d){dependents[d]=(dependents[d]||0)+1;});});',
  '',
  'var svg=byId("svg");',
  'var viewport=el("g",{id:"viewport"});',
  'var defs=el("defs");',
  'var marker=el("marker",{id:"arrow",viewBox:"0 0 10 10",refX:"5",refY:"5",markerWidth:"7",markerHeight:"7",orient:"auto"});',
  'var arrowPath=el("path",{d:"M0,0 L10,5 L0,10 z",fill:"context-stroke"});',
  'marker.appendChild(arrowPath);defs.appendChild(marker);svg.appendChild(defs);svg.appendChild(viewport);',
  '',
  'var gBoxes=el("g");var gEdges=el("g");var gNodes=el("g");',
  'viewport.appendChild(gBoxes);viewport.appendChild(gEdges);viewport.appendChild(gNodes);',
  '',
  '// module boxes',
  'var moduleEls={};',
  'moduleBoxes.forEach(function(b){',
  '  var rect=el("rect",{x:b.x,y:b.y,width:b.w,height:b.h,rx:14,class:"module-box"});',
  '  gBoxes.appendChild(rect);',
  '  var label=el("text",{x:b.x+16,y:b.y+22,class:"module-label"});',
  '  label.textContent=b.label+(b.language?"  ("+b.language+")":"");',
  '  gBoxes.appendChild(label);',
  '  moduleEls[b.id]=[rect,label];',
  '});',
  '',
  '// edges',
  'var edgeEls=[];',
  'DATA.files.forEach(function(f){',
  '  var a=pos[f.id];if(!a){return;}',
  '  f.dependencies.forEach(function(depId){',
  '    var b=pos[depId];if(!b){return;}',
  '    var midX=(a.x+b.x)/2,midY=(a.y+b.y)/2;',
  '    var edge=el("path",{d:"M"+a.x+","+a.y+" L"+midX+","+midY+" L"+b.x+","+b.y,class:"edge","marker-mid":"url(#arrow)"});',
  '    edge.__from=f.id;edge.__to=depId;',
  '    gEdges.appendChild(edge);edgeEls.push(edge);',
  '  });',
  '});',
  '',
  '// nodes',
  'var nodeEls={};',
  'DATA.files.forEach(function(f){',
  '  var p=pos[f.id];if(!p){return;}',
  '  var g=el("g",{class:"node"});g.__id=f.id;',
  '  var shape=f.isSentinel',
  '    ?el("rect",{x:p.x-R,y:p.y-R,width:R*2,height:R*2,class:"node-shape",fill:colorFor(f.kind)})',
  '    :el("circle",{cx:p.x,cy:p.y,r:R,class:"node-shape",fill:colorFor(f.kind)});',
  '  shape.__kind=f.kind;',
  '  var label=el("text",{x:p.x,y:p.y+R+13,"text-anchor":"middle"});',
  '  label.textContent=f.label;',
  '  g.appendChild(shape);g.appendChild(label);gNodes.appendChild(g);',
  '  nodeEls[f.id]=g;',
  '  g.addEventListener("mouseenter",function(ev){onEnter(f,ev);});',
  '  g.addEventListener("mousemove",function(ev){moveTip(ev);});',
  '  g.addEventListener("mouseleave",onLeave);',
  '});',
  '',
  'function edgePath(edge){var a=pos[edge.__from],b=pos[edge.__to];if(!a||!b){return "";}var midX=(a.x+b.x)/2,midY=(a.y+b.y)/2;return "M"+a.x+","+a.y+" L"+midX+","+midY+" L"+b.x+","+b.y;}',
  'function updateLayout(){',
  '  moduleBoxes.forEach(function(b){var elements=moduleEls[b.id];elements[0].setAttribute("x",b.x);elements[0].setAttribute("y",b.y);elements[0].setAttribute("width",b.w);elements[0].setAttribute("height",b.h);elements[1].setAttribute("x",b.x+16);elements[1].setAttribute("y",b.y+22);});',
  '  DATA.files.forEach(function(f){var p=pos[f.id],node=nodeEls[f.id];if(!p||!node){return;}var shape=node.childNodes[0],label=node.childNodes[1];if(f.isSentinel){shape.setAttribute("x",p.x-R);shape.setAttribute("y",p.y-R);}else{shape.setAttribute("cx",p.x);shape.setAttribute("cy",p.y);}label.setAttribute("x",p.x);label.setAttribute("y",p.y+R+13);});',
  '  edgeEls.forEach(function(edge){edge.setAttribute("d",edgePath(edge));});',
  '}',
  '',
  'function colorFor(kind){return colors[kind]||colors.static;}',
  '',
  '// ---- view transform (zoom / pan) ----',
  'var tx=0,ty=0,scale=1;',
  'function applyTransform(){viewport.setAttribute("transform","translate("+tx+","+ty+") scale("+scale+")");}',
  'function contentBounds(){',
  '  var visibleBoxes=moduleBoxes.filter(function(b){return !moduleEls[b.id][0].classList.contains("sdk-hidden");});',
  '  if(visibleBoxes.length===0){return{x:0,y:0,w:800,h:600};}',
  '  var minX=Infinity,minY=Infinity,maxX=-Infinity,maxY=-Infinity;',
  '  visibleBoxes.forEach(function(b){minX=Math.min(minX,b.x);minY=Math.min(minY,b.y);maxX=Math.max(maxX,b.x+b.w);maxY=Math.max(maxY,b.y+b.h);});',
  '  return{x:minX,y:minY,w:maxX-minX,h:maxY-minY};',
  '}',
  'function fit(){',
  '  var b=contentBounds();var rect=svg.getBoundingClientRect();var pad=60;',
  '  var k=Math.min((rect.width-pad*2)/b.w,(rect.height-pad*2)/b.h);',
  '  k=Math.max(0.08,Math.min(k,2));scale=k;',
  '  tx=(rect.width-b.w*k)/2-b.x*k;ty=(rect.height-b.h*k)/2-b.y*k;',
  '  applyTransform();',
  '}',
  'function zoomAt(cx,cy,factor){',
  '  var ns=Math.max(0.08,Math.min(scale*factor,6));',
  '  var rect=svg.getBoundingClientRect();var px=cx-rect.left,py=cy-rect.top;',
  '  tx=px-(px-tx)*(ns/scale);ty=py-(py-ty)*(ns/scale);scale=ns;applyTransform();',
  '}',
  'svg.addEventListener("wheel",function(ev){ev.preventDefault();zoomAt(ev.clientX,ev.clientY,ev.deltaY<0?1.12:0.89);},{passive:false});',
  'var panning=false,startX=0,startY=0,startTx=0,startTy=0;',
  'svg.addEventListener("mousedown",function(ev){if(ev.button!==0){return;}panning=true;svg.classList.add("panning");startX=ev.clientX;startY=ev.clientY;startTx=tx;startTy=ty;});',
  'window.addEventListener("mousemove",function(ev){if(!panning){return;}tx=startTx+(ev.clientX-startX);ty=startTy+(ev.clientY-startY);applyTransform();});',
  'window.addEventListener("mouseup",function(){panning=false;svg.classList.remove("panning");});',
  '',
  '// ---- tooltip + highlight ----',
  'var tip=byId("tooltip");var stage=byId("stage");',
  'function tagHtml(kind){var c=colorFor(kind);return "<span class=\\"tag\\" style=\\"background:"+c+"\\">"+kind+"</span>";}',
  'function esc(s){return String(s).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");}',
  'function onEnter(f,ev){',
  '  var deps=f.dependencies.length;var used=dependents[f.id]||0;',
  '  tip.innerHTML="<div class=\\"name\\">"+esc(f.label)+"</div>"+',
  '    "<div class=\\"row\\">"+tagHtml(f.kind)+" &nbsp; <b>"+esc(f.moduleId)+"</b></div>"+',
  '    "<div class=\\"row\\">Path: <b>"+esc(f.path)+"</b></div>"+',
  '    "<div class=\\"row\\">Depends on: <b>"+deps+"</b> &nbsp; Used by: <b>"+used+"</b></div>";',
  '  tip.classList.add("show");moveTip(ev);highlight(f.id);',
  '}',
  'function onLeave(){tip.classList.remove("show");clearHighlight();}',
  'function moveTip(ev){',
  '  var rect=stage.getBoundingClientRect();var x=ev.clientX-rect.left+14;var y=ev.clientY-rect.top+14;',
  '  var tw=tip.offsetWidth,th=tip.offsetHeight;',
  '  if(x+tw>rect.width){x=ev.clientX-rect.left-tw-14;}',
  '  if(y+th>rect.height){y=ev.clientY-rect.top-th-14;}',
  '  tip.style.left=x+"px";tip.style.top=y+"px";',
  '}',
  'function highlight(id){',
  '  var keep={};keep[id]=true;',
  '  var f=fileById[id];if(f){f.dependencies.forEach(function(d){keep[d]=true;});}',
  '  edgeEls.forEach(function(e){if(e.__from===id||e.__to===id){keep[e.__from]=true;keep[e.__to]=true;}});',
  '  for(var nid in nodeEls){if(keep[nid]){nodeEls[nid].classList.remove("dim");}else{nodeEls[nid].classList.add("dim");}}',
  '  if(nodeEls[id]){nodeEls[id].classList.add("hot");}',
  '  edgeEls.forEach(function(e){if(e.__from===id||e.__to===id){e.classList.add("hot");e.classList.remove("dim");}else{e.classList.add("dim");}});',
  '}',
  'function clearHighlight(){',
  '  for(var nid in nodeEls){nodeEls[nid].classList.remove("dim");nodeEls[nid].classList.remove("hot");}',
  '  edgeEls.forEach(function(e){e.classList.remove("hot");e.classList.remove("dim");});',
  '}',
  '',
  '// ---- color controls ----',
  'function bindColor(kind){',
  '  var input=byId("color-"+kind);var sw=byId("sw-"+kind);',
  '  input.value=toHex(colors[kind]);sw.style.background=colors[kind];',
  '  input.addEventListener("input",function(){',
  '    colors[kind]=input.value;sw.style.background=input.value;',
  '    var nodes=gNodes.querySelectorAll(".node-shape");',
  '    nodes.forEach(function(node){if(node.__kind===kind){node.setAttribute("fill",input.value);}});',
  '  });',
  '}',
  'function toHex(c){if(/^#[0-9a-fA-F]{6}$/.test(c)){return c;}var m=/^#([0-9a-fA-F]{3})$/.exec(c);if(m){var h=m[1];return "#"+h[0]+h[0]+h[1]+h[1]+h[2]+h[2];}return "#888888";}',
  'bindColor("static");bindColor("dynamic");',
  '',
  '// ---- SDK visibility ----',
  'var sdkHidden=false;',
  'function setSdkHidden(hidden){',
  '  sdkHidden=hidden;',
  '  layout(hidden);updateLayout();',
  '  DATA.files.forEach(function(f){if(f.isSdk&&nodeEls[f.id]){nodeEls[f.id].classList.toggle("sdk-hidden",hidden);}});',
  '  edgeEls.forEach(function(edge){var from=fileById[edge.__from],to=fileById[edge.__to];edge.classList.toggle("sdk-hidden",hidden&&((from&&from.isSdk)||(to&&to.isSdk)));});',
  '  moduleBoxes.forEach(function(box){moduleEls[box.id].forEach(function(element){element.classList.toggle("sdk-hidden",!box.visible);});});',
  '  var button=byId("toggle-sdk");button.textContent=hidden?"Show SDK":"Hide SDK";button.title=hidden?"Show SDK files":"Hide SDK files";',
  '  fit();',
  '}',
  '',
  '// ---- header + toolbar ----',
  'byId("title").textContent=CONFIG.title||"Dependency Graph";',
  'document.title=CONFIG.title||"Dependency Graph";',
  'byId("stats").textContent=DATA.files.length+" files  \\u00b7  "+DATA.modules.length+" modules  \\u00b7  "+edgeEls.length+" edges";',
  'byId("zoom-in").addEventListener("click",function(){var r=svg.getBoundingClientRect();zoomAt(r.left+r.width/2,r.top+r.height/2,1.2);});',
  'byId("zoom-out").addEventListener("click",function(){var r=svg.getBoundingClientRect();zoomAt(r.left+r.width/2,r.top+r.height/2,0.83);});',
  'byId("fit").addEventListener("click",fit);',
  'byId("toggle-sdk").addEventListener("click",function(){setSdkHidden(!sdkHidden);});',
  'if(DATA.files.length===0){byId("empty").style.display="flex";}',
  'window.addEventListener("resize",function(){/* keep current transform */});',
  'fit();',
  '})();',
].join('\n');
