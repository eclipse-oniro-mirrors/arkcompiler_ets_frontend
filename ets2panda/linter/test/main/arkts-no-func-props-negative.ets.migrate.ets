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

 // Normal function: definition by using function keyword
function foo(path: string): void {
  console.log(path);
}

foo.baz = 1;  // Property 'baz' does not exist on type 'Function'

console.log(foo.baz.toString());  // Property 'baz' does not exist on type 'Function'
console.log(foo.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(foo.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(foo.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Promise function
function fetchData(): Promise<string> {
  return new Promise<string>((resolve) => {
    setTimeout(() => resolve("Data loaded"), 1000);
  });
}
fetchData.baz = 1;

console.log(fetchData.baz.toString());  // Property 'baz' does not exist on type 'Function'
console.log(fetchData.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(fetchData.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(fetchData.caller.toString());  // Property 'caller' does not exist on type 'Function'


// async function
async function loadData(): Promise<void> {
  const data = await fetchData();
  console.log(data);
}
loadData.baz = 1;

console.log(loadData.baz.toString());  // Property 'baz' does not exist on type 'Function'
console.log(loadData.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(loadData.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(loadData.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Generic function
function genericFunction<T>(arg: T): T {
  return arg;
}
genericFunction.a = 1;
console.log(genericFunction.baz.toString());  // Property 'baz' does not exist on type 'Function'
console.log(genericFunction.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(genericFunction.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(genericFunction.caller.toString());  // Property 'caller' does not exist on type 'Function'


// Arrow function: assignment Scenarios
const arrowFunc = (path: string) => {
  console.log(path);
};

arrowFunc.bar = 2;  // Property 'bar' does not exist on type 'Function'
console.log(arrowFunc.bar.toString());  //  Property 'bar' does not exist on type 'Function'
console.log(arrowFunc.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(arrowFunc.arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(arrowFunc.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Arrow function: Anonymous Scenarios
((x : number) => x).bar = 2;

console.log(((x : number) => x).bar.toString());  //  Property 'bar' does not exist on type 'Function'
console.log(((x : number) => x).length.toString());  // Property 'length' does not exist on type 'Function'
console.log(((x : number) => x).arguments);  // Property 'arguments' does not exist on type 'Function'
console.log(((x : number) => x).caller.toString());  // Property 'caller' does not exist on type 'Function'

// Arrow function: Promise Scenarios
const fetchDataArrow = (): Promise<string> => {
  return new Promise<string>((resolve, reject) => {
    setTimeout(() => {
      const success = true;
      if (success) {
        resolve("success");
      } else {
        reject("fail");
      }
    }, 1000);
  });
};
fetchDataArrow.bar = 2;

console.log(fetchDataArrow.bar.toString()); // Property 'bar' does not exist on type 'Function'
console.log(fetchDataArrow.length.toString()); // Property 'length' does not exist on type 'Function'
console.log(fetchDataArrow.arguments); // Property 'arguments' does not exist on type 'Function'
console.log(fetchDataArrow.caller.toString()); // Property 'caller' does not exist on type 'Function'

// Arrow function: async Scenarios
const loadDataArrow = async (): Promise<void> => {
  try {
    const result : string = await fetchDataArrow();
    console.log(result);
  } catch (error) {
    console.error(error);
  }
};
fetchDataArrow.bar = 2;

console.log(loadDataArrow.bar.toString()); // Property 'bar' does not exist on type 'Function'
console.log(loadDataArrow.length.toString()); // Property 'length' does not exist on type 'Function'
console.log(loadDataArrow.arguments); // Property 'arguments' does not exist on type 'Function'
console.log(loadDataArrow.caller.toString()); // Property 'caller' does not exist on type 'Function'


// Class Method
class A {
  foo3(path: string): void {
  }
}

let a : A = new A();
const classMethod = a.foo3;
classMethod.var = 1;
console.log(classMethod.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(classMethod.arguments);   // Property 'arguments' does not exist on type 'Function'
console.log(classMethod.caller.toString());  // Property 'caller' does not exist on type 'Function'


// Interface Method
interface L {
  foo1(par: number, par1: string): void;
}

class MyClass implements L {
  foo1(par: number, par1: string): void {
  }
}

let obj: L = new MyClass();
const interfaceMethod = obj.foo1
interfaceMethod.a = 1;
console.log(interfaceMethod.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(interfaceMethod.arguments);   // Property 'arguments' does not exist on type 'Function'
console.log(interfaceMethod.caller.toString());  // Property 'caller' does not exist on type 'Function'


// Std 'Function' type
const stdFunc: Function  = (path: string) => {
  console.log(path);
};
stdFunc.bar = 2;

console.log(stdFunc.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(stdFunc.arguments);   // Property 'arguments' does not exist on type 'Function'
console.log(stdFunc.caller.toString());  // Property 'caller' does not exist on type 'Function'

// Functional type
type FuncType = (path: string) => void;
let ft: FuncType = (path: string): string => {
  return path + ': foo.bar';
};
ft.bar=2;

console.log(ft.length.toString());   //Property 'length' does not exist on type 'Function'
console.log(ft.caller.toString());  //  Property 'caller' does not exist on type 'Function'

// Functional type alias
type FT2 = FuncType;
let ft2: FT2 = (path: string): string => {  // Type '((p1: String) => String)' cannot be assigned to type '((p1: String) => void)'
  return path + ': foo.bar';
};
ft2.bar=2;

console.log(ft2.length.toString()); //Property 'length' does not exist on type 'Function'
console.log(ft2.caller.toString()); //  Property 'caller' does not exist on type 'Function'


// Exported Function
export function validate(data: string): boolean {
  return data.length > 0;
}
validate.error = 'Invalid data format'; // Property 'error' does not exist on type 'Function'


// Static Method
class MathUtils {
  static square(x: number): number {
    return x * x;
  }
}

const staticMethod = MathUtils.square;
staticMethod.a = 1;
console.log(staticMethod.length.toString());  // Property 'length' does not exist on type 'Function'
console.log(staticMethod.arguments);   // Property 'arguments' does not exist on type 'Function'
console.log(staticMethod.caller.toString());  // Property 'caller' does not exist on type 'Function'