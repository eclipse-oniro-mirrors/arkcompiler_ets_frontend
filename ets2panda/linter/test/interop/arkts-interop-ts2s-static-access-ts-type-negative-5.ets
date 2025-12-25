/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
import {
  a,
  a1,
  a2,a3,a4,a5,
  a6,a7,
  b,
  c,
  d,
  ET,
  e,
  e2,
  e3,
  X,
  f,
  g,
  gt,
  g2,
  g3,
  h,
  i,
  j,
  k,
  K1,
  K2,
  l,l1,
  l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13,l14,l15,
  m,
  IndexedAccessType,
  IndexedAliasType,
  n,
  ConditionalType,
  o,
  MappedType,
  TXY,
  p,
  TemplateType,
  q,
  QT,
  qv,
  r,
  s,
  t,
  u,
  v,
  w,
  x,
  y,
  z,
  aa,
  ab,
  ac,
  NS,
  MD
} from "./ignore_files/unique_types4";

// any
a.log("a"); 
console.log("" + a1); 
a2;  
console.info('a7' + a7);  

// unknown
b; 

// symbol
c; 

// Function
d; 

// object literal
e; 
e2; 
e3; 
let ee: ET = { x: 3, y: 'world' }; 

// mixing enum
X.a === 0;  
const ff = X.b;  

// call signature
g(2); 
let gg: gt; 
g3(); 

// constructor signature
new h(); 

// index signature
i; 

// intersection
j; 

// keyof
let k1: K1; 

// typeof 
l.x; 

let ll1: l1; 

new l14().prop; 

console.info("" + l); 

function fl(arg: l1){}; 

interface IL1 {
  filed: l1; 
}

function IL2(): l1 { 
  return {x:1, y:"2"};
}

// complex
let tl6: l6; 
let tl7: l7; 
let tl8: l8<string>;  
let tl9: l9<string>;  
let tl10: l10; 
let tl11: l11; 
let tl12: l12; 

// indexed access type
m; 
let mm: IndexedAccessType; 
let mm2: IndexedAliasType; 

// conditional types
n; 
let nn: ConditionalType<'A'> = 'B'; 

// mapped types
o; 
const oo: MappedType<TXY> = { x: 5, y: 'hello' }; 

// template literal types
p; 
const pp: TemplateType = 'A_id'; 

// utils
q;  
let qvv: QT;  
r; 
s; 
t; 
u; 
v; 
w; 
x; 
y; 
z; 
aa; 
ab; 
ac; 