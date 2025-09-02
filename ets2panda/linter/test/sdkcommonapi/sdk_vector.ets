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

import Vector from './@ohos.util.Vector';

let vector : Vector<number> = new Vector(); //error
vector.add(2); //error
vector.add(4); //error
vector.add(5); //error
vector.add(4); //error
// 使用方法一：
let nums: Array<number> =  vector.convertToArray() //error
for (let item of nums) {
  console.log("testLog value:" + item);
}
vector.clear(); //error
vector.add(2); //error
vector.clone(); //error
vector.add(2); //error
vector.convertToArray(); //error
vector.add(2); //error
let arr: Array<number> = []
vector.copyToArray(arr); //error


vector.add(2); //error
vector.forEach((value: number, index?: number) => { //error
  console.log('value = ' + value, 'index = ' + index);
});
vector.add(2); //error
vector.get(0); //error
vector.add(2); //error
vector.getCapacity(); //error
vector.getFirstElement(); //error
vector.getIndexFrom(2, 0); //error
vector.getIndexOf(2); //error
vector.getLastElement(); //error
vector.getLastIndexFrom(2, 0); //error
vector.getLastIndexOf(2); //error
vector.has(2); //error
vector.increaseCapacityTo(2); //error
vector.insert(2, 0); //error
vector.isEmpty(); //error
vector.remove(2); //error
vector.removeByIndex(0); //error
vector.removeByRange(0, 1); //error
vector.replaceAllElements((value: number) => { //error
  return value;
});
vector.set(0, 0); //error
vector.setLength(0); //error
vector.sort((firstValue: number, secondValue: number) => { //error
  return firstValue - secondValue;
});
vector.subVector(0, 1); //error
vector.toString(); //error
vector.trimToCurrentLength(); //error
let result = vector.length; //error
//sum:40