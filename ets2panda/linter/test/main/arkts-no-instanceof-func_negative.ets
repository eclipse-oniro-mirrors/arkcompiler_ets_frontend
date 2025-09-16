/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


import foo5 from './arkts-no-instanceof-func_positive'
import {foo3} from './arkts-no-instanceof-func_positive'
import {foo3 as foo6} from './arkts-no-instanceof-func_positive'
import {BB} from './arkts-no-instanceof-func_positive'
import {BB as CC} from './arkts-no-instanceof-func_positive'

function foo1() {

}

console.log('' + ((new foo1()) instanceof foo1)) // report error for instanceof a function.

let a = foo1;
console.log('' + ((new foo1()) instanceof a)) // report error for instanceof a function which is assigned to a.

let b = a;
let c = b;
let d = c;
console.log('' + ((new a()) instanceof d)) // report error for instanceof a function which is assigned to a b c d.

namespace AA {
  export function foo2() {}
}

console.log('' + ((new foo1()) instanceof AA.foo2)) // report error for instanceof a namespace export function.

let e = AA.foo2;
console.log('' + ((new foo1()) instanceof e)) // report error for instanceof a function which is assigned to e.


console.log('' + ((new foo1()) instanceof foo5)) // report error for instanceof a function which is default import.

console.log('' + ((new foo1()) instanceof foo3)) // report error for instanceof a function which is import.

console.log('' + ((new foo1()) instanceof foo6)) // report error for instanceof a function which is alias import.

let foo7 = foo5;
console.log('' + ((new foo1()) instanceof foo7)) // report error for instanceof a function which is import and assigned to foo7.

let foo8 = foo3;
console.log('' + ((new foo1()) instanceof foo8)) // report error for instanceof a function which is import and assigned to foo8.

let foo9 = foo6;
console.log('' + ((new foo1()) instanceof foo9)) // report error for instanceof a function which is alias import and assigned to foo9.

let foo10 = foo9;
console.log('' + ((new foo1()) instanceof foo10)) // report error for instanceof a function which is import and assigned to foo9 and foo10.

console.log('' + ((new foo1()) instanceof BB.foo11)) // report error for instanceof a function which is import from namespace.

console.log('' + ((new foo1()) instanceof CC.foo11)) // report error for instanceof a function which is import from namespace.