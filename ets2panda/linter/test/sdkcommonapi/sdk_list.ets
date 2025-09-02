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

import { ArrayList } from './@ohos.util.ArrayList';
import LinkedList from './api/@ohos.util.LinkedList';
import {PlainArray} from './api/@ohos.util.PlainArray';
import TreeMap from './api/@ohos.util.TreeMap';

let linkedList: LinkedList<number> = new LinkedList();
linkedList.add(4);
linkedList.add(5);
linkedList.add(4);
let result = linkedList.removeFirstFound(6);//error
let result1 = linkedList.removeLastFound(6);  //error

let plainArray: PlainArray<number> = new PlainArray<number>();
let result3 = plainArray.getKeyAt(-1);
let result32 = plainArray.getValueAt(-1);
let result4 = plainArray.setValueAt(-2, 0);

let treeMap: TreeMap<string, string> =
  new TreeMap<string, string>((firstValue: string, secondValue: string): boolean => { //error
    return firstValue > secondValue
  });

let arrayList: ArrayList<String|Number> = new ArrayList<String|Number>();
arrayList.add("刘");
arrayList.add("张三");
arrayList.add(1);
arrayList.add(2);
arrayList.sort((a, b) => a - b);  //error

function fn() {console.log('Hello');}
let arrayList2: ArrayList<number> = new ArrayList<number>();
arrayList2.add(2);
arrayList2.forEach((value: number, index?: number) => { //error
  console.log('value:' + value, 'index:' + index);
}, fn);

let arrayList3: ArrayList = new ArrayList();
arrayList.add(2);
arrayList.add(4);
arrayList.replaceAllElements((value: number): number => { //error
  return value;
},fn);


arrayList.replaceAllElements((value: number): number => { //error need fix on whitelist
  return value;
});
arrayList2.forEach((value: number, index?: number) => { //error need fix on whitelist
  console.log('value:' + value, 'index:' + index);
});
//sum:6