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

class TestHelper {
  test(arg0: () => boolean, arg1: string) {
    throw new Error("Method not implemented.");
  }
  constructor(name: string) {
  }
}

export function test_unique_type(testCaseRet: AllTypes) {
  let test_helper = new TestHelper("TEST_UNIQUE_TYPE");
  
  test_helper.test(() => {

    // any
    a.log("a"); // error
    console.log("" + a1); // error
    a2; // error 
    a3; 
    a4("test"); 
    a5("test"); 
    console.info('a6' + a6);
    console.info('a7' + a7); // error 

    // unknown
    b; // error

    // symbol
    c; // error

    // Function
    d; // error

    // object literal
    e;
    e2;
    e3;
    let ee: ET = { x: 3, y: 'world' };

    // mixing enum
    f; // error
    X.a === 0; // error 
    const ff = X.b; // error 

    // call signature
    g(2); // error
    let gg: gt; // error
    g2(); 
    g3();

    // constructor signature
    new h(); // error

    // index signature
    i; // error

    // intersection
    j; // error

    // keyof
    let k1: K1; 
    let k2: K2;
    k;

    // typeof 
    l.x; // error

    let ll1: l1; // error

    new l14().prop; // error

    console.info("" + l); // error

    function fl(arg: l1){}; // error

    interface IL1 {
      filed: l1; // error
    }

    function IL2(): l1 { // error
      return {x:1, y:"2"};
    }

    l2;

    // complex 
    let tl3: l3;
    let tl4: l4;
    let tl5: l5;
    let tl6: l6; // error
    let tl7: l7; // error
    let tl8: l8<string>; // error 
    let tl9: l9<string>; // error 
    let tl10: l10; // error
    let tl11: l11; // error
    let tl12: l12; // error
    l13({x:1,y:"2"});
    new l14();
    let ll15: l15;


    // indexed access type
    m; 
    let mm: IndexedAccessType; // error
    let mm2: IndexedAliasType;

    // conditional types
    n; 
    let nn: ConditionalType<'A'> = 'B'; // error

    // mapped types
    o; 
    const oo: MappedType<TXY> = { x: 5, y: 'hello' }; // error

    // template literal types
    p; 
    const pp: TemplateType = 'A_id'; // error

    // utils
    q; // error 
    qv;
    let qvv: QT;
    r; // error
    s; // error
    t; // error
    u; // error
    v; // error
    w; // error
    x; // error
    y; // error
    z; // error
    aa; // error
    ab; // error
    ac; // error

    // namespace
    let ii1: NS.ns_interface; 
    let ii2: NS.X; 

    // module
    let ii3: MD.md_intereface; 
    let ii4: MD.X; 

    return typeof a === "any"   //error
  }, "all types ")
}