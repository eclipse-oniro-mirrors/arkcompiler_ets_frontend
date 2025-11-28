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

function foo(path: string): void {
  console.log(path);
}

foo.baz = 1;  // Property 'baz' does not exist on type 'Function'

console.log(foo.baz.toString());  // Property 'baz' does not exist on type 'Function'
console.log(foo.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(foo.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(foo.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Arrow function
const foo2 = (path: string) => {
  console.log(path);
};

foo2.bar = 2;  // Property 'bar' does not exist on type 'Function'
console.log(foo2.bar.toString());  //  Property 'bar' does not exist on type 'Function'
console.log(foo2.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(foo2.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(foo2.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Std 'Function' type
const stdFunc: Function  = (path: string) => {
  console.log(path);
};

console.log(stdFunc.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(stdFunc.arguments);   // Property 'arguments' does not exist on type 'Function'
console.log(stdFunc.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Functional type
type FuncType = (path: string) => void;
let ft: FuncType = (path: string): string => {
  return path + ': foo.bar';
};

console.log(ft.length.toString());   //Property 'length' does not exist on type 'Function'
console.log(ft.caller.toString());  //  Property 'caller' does not exist on type 'Function'

// Functional type alias
type FT2 = FuncType;
let ft2: FT2 = (path: string): string => {  // Type '((p1: String) => String)' cannot be assigned to type '((p1: String) => void)'
  return path + ': foo.bar';
};

console.log(ft2.length.toString()); //Property 'length' does not exist on type 'Function'
console.log(ft2.caller.toString()); //  Property 'caller' does not exist on type 'Function'
// Standard Function Declaration
function standardFunc(path: string): void {
  console.log(path);
}
standardFunc.baz = 1; // Property 'baz' does not exist on type 'Function'
standardFunc._config = 0; // Property '_config' does not exist on type 'Function'

// Arrow Function
const arrowFunc = (path: string) => {
  console.log(path);
};
arrowFunc.bar = 2; // Property 'bar' does not exist on type 'Function'

// Exported Function
export function validate(data: string): boolean {
  return data.length > 0;
}
validate.error = 'Invalid data format'; // Property 'error' does not exist on type 'Function'
