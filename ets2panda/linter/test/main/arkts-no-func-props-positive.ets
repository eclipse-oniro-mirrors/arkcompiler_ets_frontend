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

// Definition field in calss
export class T1 {
  static foo(path: string): void {
    console.info(path);
  }

  static baz: number = 2;
}

T1.foo("example");
console.info(`baz value: ${T1.baz}`);

// Normal function: definition by using function keyword
function foo(path: string): void {
  console.log(path);
}
console.info(`foo.name: ${foo.name}`);

// Arrow function: assignment Scenarios
const arrowFunc = (path: string) => {
  console.log(path);
};
console.info(`arrowFunc.name: ${arrowFunc.name}`);

// Arrow function: Anonymous Scenarios
((x : number) => x).name;

// Class Method
class A {
  foo3(path: string): void {
  }
}

let a : A = new A();
const classMethod = a.foo3;
console.info(`classMethod.name: ${classMethod.name}`);

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
interfaceMethod.name;
console.info(`interfaceMethod.name: ${interfaceMethod.name}`);

// Std 'Function' type
const stdFunc: Function  = (path: string) => {
  console.log(path);
};
console.info(`stdFunc.name: ${stdFunc.name}`);

// Functional type
type FuncType = (path: string) => string;
let ft: FuncType = (path: string): string => {
  return path + ': foo.bar';
};
console.info(`ft.name: ${ft.name}`);

// Functional type alias
type FT2 = FuncType;
let ft2: FT2 = (path: string): string => {
  return path + ': foo.bar';
};
console.info(`ft2.name: ${ft2.name}`);

//Exported Function
export function validate(data: string): boolean {
 return data.length > 0;
}
console.info(`Exported Function.name: ${validate.name}`);


// Static Method
class MathUtils {
  static square(x: number): number {
    return x * x;
  }
}

const staticMethod = MathUtils.square;
console.info(`staticMethod.name: ${staticMethod.name}`);