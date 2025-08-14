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
//NumberConstructor
const num1 = new Number('123');//BuiltinNewCtor 1
Number('123');//BuiltinNoCtorFunc 1
console.log(Number(undefined).toString());//BuiltinNoCtorFunc 1
Number.isFinite(1)  //BuiltinAll 1
Number.isInteger(true) //BuiltinAll 1
Number.isNaN(true) //BuiltinAll 1
Number.isSafeInteger(true) //BuiltinAll 1
isFinite(11);
//ObjectConstructor
interface Person {
  name: string;
  age: number;
}
const obj: Person = {
  name: '',
  age: 42
};
const a: [string, number][] = Object.entries(obj);//BuiltinAll 1
Object.values(obj);//BuiltinAll 1

const entries: Map<string, string> = new Map([
  ['1', '2']
]);
const obj1 = Object.fromEntries(entries);//BuiltinAll 1
typeof new Object();//BuiltinAll 1

//String
new String(undefined);//BuiltinNewCtor 1
String('');//BuiltinNoCtorFunc 1
const str: string = 'abc';
str.replace('a', 'b');
str.replaceAll('a', 'b');
class Demo implements String{
  replaceAll1(searchValue: string | RegExp, replacer: string): string {

  }
  replaceAll(searchValue: string | RegExp, replacer: (substring: string, ...args: any[]) => string): string{//BuiltinAll 1

  }
}
//replace(searchValue: string | RegExp, replaceValue: string): string;
const text = "Hello World, World!";
const result1 = text.replace("World", "TypeScript");
const result2 = text.replace(/World/g, "TypeScript");

//replace(searchValue: string | RegExp, replacer: (substring: string, ...args: any[]) => string): string;
function toUpperCaseReplacer(match: string): string {
  return match.toUpperCase();
}
text.replace('',toUpperCaseReplacer)//BuiltinAll 1
const result4 = text.replace("world", (match) => match.toUpperCase());//BuiltinAll 1
const result5 = text.replace("/(\w+) (\w+)/", (first, last:number) => `${last}, ${first}`);//BuiltinAll 1
//replace(searchValue: { [Symbol.replace](string: string, replacer: (substring: string, ...args: any[]) => string): string; }, replacer: (substring: string, ...args: any[]) => string): string;
//lib.es2015.symbol.wellknown.d.ts
const result6 = text.replace(/-/g, (match, offset:string) => `-${offset}-`);//BuiltinAll!!
const camelCase = text.replace(/_(\w)/g, (_, char:string) => char.toUpperCase());//BuiltinAll!!
console.log(camelCase);
const prices = { apple: 5, banana: 3 };
function pricesChange(count:string, fruit:string){
  const price = prices[fruit as keyof typeof prices] || 0;
  return `${count} ${fruit} ($${price * parseInt(count)})`;
}
const result7 = text.replaceAll(/(\d+)\s+(\w+)/g, pricesChange);//BuiltinAll 1

//WeakMapConstructor
interface Person1 {
  age: number;
}
const key1: Person1 = {age: 12};
const key2: Person1 = {age: 123};
const weakMap = new WeakMap<object, string>([ // 0 lib.es2015.iterable.d.ts
  [key1, 'value1'],
  [key2, 'value2']
]);
//JSON
const a = JSON.stringify({x: 5, y: 6});//BuiltinAll 1
class Test implements JSON{
  stringify(value: string, replacer?: (number | string)[] | null, space?: string | number): string{//BuiltinAll 1
    return '';
  }
  str: String = '';
  get() {
    return this.str;
  }
}
const d = new Test();
d.get().replaceAll('a', pricesChange);//BuiltinAll 1

const boolObj1 = new Boolean(true); //BuiltinNewCtor
console.log(boolObj1.valueOf());
console.log(new Test().valueOf()); //BuiltinAll
console.log(typeof boolObj1);
const tt = new Test();
typeof tt.valueOf; //BuiltinAll??
const boolObj2 = new Boolean(""); //BuiltinNewCtor
const boolObj3 = new Boolean(null); //BuiltinNewCtor
const boolObj4 = new Boolean(0); //BuiltinNewCtor

//sum:27
