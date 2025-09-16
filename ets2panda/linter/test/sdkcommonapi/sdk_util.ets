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

//util.Base64
let base64 = new util.Base64();  //error
let array = new Uint8Array([99,122,69,122]);
base64.decode(array).then((val) => {  //error
  console.info(val.toString()); // 115,49,51
})
let buff = 'czEz';
let result0 = base64.decodeSync(buff);  //error
console.info("result0 = " + result0); // result = 115,49,51"

base64.encode(array).then((val) => {  //error
  console.info(val.toString()); // 99,122,69,122
})

let result1 = base64.encodeSync(array);  //error
console.info("result1 = " + result1); // result = 99,122,69,122"

base64.encodeToString(array).then((val) => {  //error
  console.info(val); // czEz
})

let result2 = base64.encodeToStringSync(array);  //error
console.info("result2 = "+ result2); // result = czEz
//util.LruBuffer
class ChildLruBuffer<K, V> extends util.LruBuffer<K, V> {  //error
  length: number=0; //error
  constructor(capacity?: number) {
    super(capacity); //error
  }
  afterRemoval(isEvict: boolean, key: K, value: V, newValue: V): void { //error
    if (isEvict === true) {
      console.info('key: ' + key); // 输出结果：key: 11
      console.info('value: ' + value); // 输出结果：value: 1
      console.info('newValue: ' + newValue); // 输出结果：newValue: null
    }
  }
  [Symbol.iterator](): IterableIterator<[ //error
    K,
    V
  ]>{
    return <>;
  }
}
class ChildLruBuffer1<K, V> extends util.LruBuffer<K, V> { //error
}
let lru: ChildLruBuffer<number, number> = new ChildLruBuffer(2);
lru.put(11, 1);  //error
lru.put(22, 2);  //error
lru.afterRemoval(false,22, 2,1);

let lru1: ChildLruBuffer1<number, number> = new ChildLruBuffer1(2);
lru1.afterRemoval(false,22, 2,1);  //error

let pro : util.LruBuffer<number,number> = new util.LruBuffer();  //error*2
pro[Symbol.iterator]();  //error
pro.afterRemoval(true,1,2,1);  //error
pro.length;  //error
pro.clear();  //error
let result4 = pro.contains(20);  //error
console.info('result4 = ' + result4); // result = false
pro.createDefault(50);  //error
pro.put(2,10);  //error
pro.entries();  //error
pro.get(2);  //error
pro.remove(20);  //error
console.info("result = " + pro.toString());   //error
pro.updateCapacity(100);  //error

let pro3: util.LruBuffer<number | string,number | string> = new util.LruBuffer();  //error*2
pro3.put(2,10);  //error
pro3.put(2,"anhu");  //error
pro3.put("afaf","grfb");  //error
let result3 = pro3.values();  //error
console.info("result3 = " + result3); // result = anhu,grfb"

let pro4: util.LruBuffer<number,number>= new util.LruBuffer();  //error*2
pro4.put(2,10);  //error
pro4.put(1,8);  //error
console.info("result = " + pro4.length);   //error

//util.Scope
class ScopeDemo extends  util.Scope {}   //error
class Temperature implements util.ScopeComparable {
  private readonly _temp: number;
  constructor(value: number) {
    this._temp = value;
  }
  compareTo(value: Temperature) {
    return this._temp >= value.getTemp();
  }
  getTemp() {
    return this._temp;
  }
  toString(): string {
    return this._temp.toString();
  }
}
let tempLower = new Temperature(30);
let tempUpper = new Temperature(40);
let tempMiDF = new Temperature(35);
let range = new util.Scope(tempLower, tempUpper);  //error
console.info("result = " + range.clamp(tempMiDF));  //error
console.info("range = " + range); // range = [30, 40]"
let tempLess = new Temperature(20);
let tempMore = new Temperature(45);
let rangeSec = new util.Scope(tempLess, tempMore);  //error
range.contains(rangeSec);  //error

let tempMiDS = new Temperature(39);
range.expand(tempMiDF, tempMiDS);  //error
range.expand(range);  //error
console.info("result = " + result);
range.expand(tempMiDF);  //error
console.info("result = " + result);
range.getLower();  //error
range.getUpper();  //error
range.intersect(tempMiDF, tempMiDS);  //error
range.intersect(range);  //error
range.toString();  //error

//sum:54