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
import util from './@ohos.util';

function test(obj: Object) {
  let type = new util.types();
  type.isArgumentsObject(obj); //error
  type.isGeneratorFunction(obj); //error
  type.isGeneratorObject(obj); //error
  type.isModuleNamespaceObject(obj); //error
  type.isProxy(obj); //error
  type.isSymbolObject(obj); //error
}

class A {
}

util.Aspect.addAfter(A, 'foo', true, (str: string) => { //error
});
util.Aspect.addBefore(A, 'foo', true, (str: string) => { //error
});
util.Aspect.replace(A, 'foo', true, (str: string) => { //error
});

let base64Helper = new util.Base64Helper();
let array = new Uint8Array([115, 49, 51]);
let result = base64Helper.encodeToStringSync(array); //error
let cache = new util.LRUCache<number, number>(); //error
let result1 = cache.put(2, 10); //error
console.log('result = ' + result1); // 输出结果：result = 10
let pro2: util.LRUCache<number, number> = new util.LRUCache(); //error
let result5 = pro2.getCapacity(); //error
console.info("result5 = " + result5);
pro2.put(1, 8); //error
let result6 = pro2.getCreateCount(); //error
console.info("result6 = " + result6);
pro2.getMatchCount(); //error
pro2.get(2);
console.info("result = " + pro2.getMissCount());  //error
console.info("result = " + pro2.getPutCount());  //error
pro2.updateCapacity(2); //error
pro2.put(50, 22); //error
pro2.length; //error
pro2.getCapacity(); //error
console.info("result = " + pro2.getRemovalCount()); //error
console.info("result = " + pro2.isEmpty());
console.info("result = " + pro2.keys()); // 输出结果：result = 2"
console.info("result = " + pro2.put(2, 10)); //error
console.info("result = " + pro2.remove(20));
let a: number = 5;
new util.LRUCache().length; //error*2

let errnum = -1;
console.info("" + util.getErrorString(errnum)); //error

let res = util.printf("\"%s\"", "hello world!"); //error
console.info(res); // 输出结果：hello world!

async function fn() { return 'hello world'; }
const addCall = util.promiseWrapper(util.callbackWrapper(fn)); //error

let decodeWithStreamOptions: util.DecodeWithStreamOptions = { stream: false } //error
let textDecoder1 = new util.TextDecoder("utf-8",{ignoreBOM: true}); //error

let uint8 = new Uint8Array(6);
uint8[0] = 0xEF;
uint8[1] = 0xBB;
uint8[2] = 0xBF;
uint8[3] = 0x61;
uint8[4] = 0x62;
uint8[5] = 0x63;
console.info("input num:");
textDecoder1.decode(uint8, {stream: false}); //error

let textDecoderOptions: util.TextDecoderOptions = {
  fatal: false,
  ignoreBOM : true
}

let textEncoder3: util.TextEncoder = new util.TextEncoder();
let resu = textEncoder3.encode(''); //error

let textEncoder: util.TextEncoder = new util.TextEncoder();
let buffer = new ArrayBuffer(4);
let uint = new Uint8Array(buffer);
let result10 = textEncoder.encodeInto('', uint); //error

//sum:35