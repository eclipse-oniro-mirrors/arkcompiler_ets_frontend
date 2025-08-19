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

// ---------------------
// Shared types
// ---------------------
interface IA {
  value: number;
  name?: string;
}

interface IB {
  value: number;
}

class CA {
  value: number = 1;
}

class CB {
  value: number = 1;
}

// ---------------------
// Allowed Cases
// ---------------------

// ----------- Allowed: simple type, no union -------------
let ok1: IA = { value: 1 }; // OK — no union

// ----------- Allowed: nullish unions --------------------
let ok2: IA | null = { value: 1 }; // OK — null union
let ok3: IA | undefined = { value: 1 }; // OK — undefined union
let ok4: IA | null | undefined = { value: 1 }; // OK — null + undefined union

// ----------- Allowed: casted unions ------------------------
let ok5: IA | IB = { value: 1 } as IA; // OK — explicit cast

// ----------- Built-in lib.* guard: should be allowed --------------------
let ok6: Record<string, object> | object[] = {}; // OK — both are lib.* types
let ok7: Promise<any> | string[] = {}; // OK — Promise and Array are from lib.*

// ----------- Other allowed scenarios ---------------------

// Using 'as' cast
function ok8(): IA | CB {
  return { value: 1 } as IA; // ok
}

// Passing nullish or undefined unions — allowed
function ok9(a: IA | null | undefined) {}
ok9({ value: 42 }); // ok

// Passing a variable declared as IA
const objIA: IA = { value: 1 };
function ok10(a: IA | IB) {
  console.log(a.value);
}
ok10(objIA); // ok

// ---------------------
// Disallowed Cases
// ---------------------

// ----------- Disallowed: simple union of interface + interface ------------
let fail1: IA | IB = { value: 1 }; // ❌ Error — object literal into interface union

// ----------- Disallowed: interface + class -----------------
let fail2: IA | CA = { value: 1 }; // ❌ Error

// ----------- Disallowed: simple union of class + class ------------------
let fail3: CA | CB = { value: 1 }; // ❌ Error

// ----------- Disallowed: with nullish in mixed but still failing --------------------
let fail4: IA | IB | undefined = { value: 1 }; // ❌ Error — nullish doesn’t save it here

// ----------- Disallowed: function return types ---------------------

// Return object literal to interface|interface without cast
function err1(): IA | IB {
  return { value: 1 }; // ❌ error
}

// Return object literal to class|interface without cast
function err2(): CB | IA {
  return { value: 1 }; // ❌ error
}

// ----------- Disallowed: function param passing ---------------------

// Pass object literal to interface|interface param without cast
function err3(a: IA | IB) {
  console.log(a.value);
}
err3({ value: 42 }); // ❌ error

// Pass object literal to class|class param without cast
function err4(a: CA | CB) {
  console.log(a.value);
}
err4({ value: 10 }); // ❌ error

// Mixed interface|class param without cast
function err5(a: IA | CB) {
  console.log(a.value);
}
err5({ value: 5 }); // ❌ error

// Nested in union with undefined — still error
function err6(a: IA | CB | undefined) {}
err6({ value: 99 }); // ❌ error
