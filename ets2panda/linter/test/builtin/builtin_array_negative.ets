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
import { collections } from '@kit.ArkTS';

let arr1 = new Array<number>(); //BuiltinNewCtor
let arr2 = new Array<number>(); //BuiltinNewCtor
arr1.concat(1, arr2); //BuiltinAll
Array.length;
let arr11: ReadonlyArray<number> = new Array<number>(); //BuiltinNewCtor
let arr22: ReadonlyArray<number> = new Array<number>(); //BuiltinNewCtor
arr11.concat(1, arr22); //BuiltinAll

const isArr = Array.isArray([1]); //BuiltinAll
let arr = new Array<string>(); //BuiltinNewCtor

const date = new Date("August 19,1975 23:00:00 UTC");//lib.es2015.core.d.ts
const jsonDate = date.toJSON(); //BuiltinAll
new Date().toJSON(); //BuiltinAll+BuiltinNewCtor
function getDate(){
  return date;
}
getDate().toJSON(); //BuiltinAll
console.log(new Demo().localDate?.toJSON()) //BuiltinAll
const demo = new Demo();
demo.get()?.toJSON(); //BuiltinAll
Date.toString();
//Reflect
let a1 = new Array<number>(1,2,3) //BuiltinNewCtor
Reflect.ownKeys(a1) //BuiltinAll

//ArrayBufferConstructor
ArrayBuffer.isView(1); //BuiltinAll
ArrayBuffer.isView(100n); //BuiltinAll
interface ArrayBufferConstructor{}
class Demo implements ArrayBufferConstructor,Date{
  localDate:Date|undefined = undefined;
  set(localDate:Date|undefined){
    localDate = new Date(); //BuiltinNewCtor
    return localDate
  }
  get():Date|undefined{
    return this.set(this.localDate);
  }
  isView(arg: string): arg is ArrayBufferView {
    this.localDate?.toJSON(); //BuiltinAll
  }
  toJSON(key?: number): string { //BuiltinAll
    return '';
  }
}

let array = new collections.Array(1, 2, 3);
let array1 = new collections.Array(4, 5, 6);
let array2 = new collections.Array(7, 8, 9);
let concatArray = array.concat(array1, array2);
let arr3: collections.Array<string> = new collections.Array('a', 'b', 'c', 'd');
let result: boolean = collections.Array.isArray(arr3);
console.info(result + '');

const uint8 = new Uint8Array([1, 2, 3]);
ArrayBuffer.isView(uint8) //BuiltinAll
const buffer = new ArrayBuffer(16);
const dataView = new DataView(buffer);
ArrayBuffer.isView(dataView) //BuiltinAll

//sum:24