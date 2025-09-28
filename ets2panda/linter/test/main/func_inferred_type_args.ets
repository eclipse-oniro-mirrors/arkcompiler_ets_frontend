/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

import { bad_func } from "./dynamic_lib"
import { Deque } from '@kit.ArkTS';
import ArrayList from '@ohos.util.ArrayList';
import Deque from '@ohos.util.Deque';
import HashMap from '@ohos.util.HashMap';
import HashSet from '@ohos.util.HashSet';
import LinkedList from '@ohos.util.LinkedList';
import PlainArray from '@ohos.util.PlainArray';
import Queue from '@ohos.util.Queue';
import TreeMap from '@ohos.util.TreeMap';
import TreeSet from '@ohos.util.TreeSet';
import { LinkedList } from '@kit.ArkTS';
function choose<T>(x: T, y: T): T {
  return Math.random() < 0.5 ? x : y
}
let x = choose(10, 20);

function greet<T>(): T {
  return 'hello' as T;
}
let y = greet(); // T is 'unknown'

function foo<K, N = any>() {
  return 3;
}
foo(); // K is 'unknown'
foo<string>(); // N is 'any'
foo<number, string>();

function bar<T>(): T[] {
  return [];
}
bar(); // T is 'unknown'
bar<string>();

function baz<T>(m: Map<T, string>) {}
baz(new Map()); // T is 'any'
baz(new Map<number, string>());

function foobar<T>(t: T) {}
let z: any;
foobar(new Set<any>()); // T is 'Set<any>'
foobar([z]); // T is 'any[]'
foobar({a: 1, b: 2}); // T is Object literal type

let set = new Set(); // Type argument of constructor is 'unknown'
class C<T> {
  constructor(t?: T);
  constructor() {}
}
let c = new C(); // T is 'unknown'
c = new C<number>();
c = new C(bar()); // T is 'unknown[]'
c = new C(set); // T is 'Set<unknown>'
c = new C(new Set<any>()); // T is 'Set<any>'

const d: { ddd: [[]] };
d.ddd.push([]);

{
  let a: Promise<number> | undefined = undefined
  let b: undefined = undefined
  let c: [string, number] = ["abc", 0]
  let d: [any, number] = [10, 10]

  Boolean(a)
  Boolean(b)
  Boolean(c)
  Boolean(d)
}

Boolean(bad_func())

let arr1 = new Array();
let map1 = new Map();
let readonlyArr1 = new ReadonlyArray();
let promise1 = new Promise((resolve, reject) => {});
let weakMap1 = new WeakMap();
let weakSet1 = new WeakSet();

class Box1<T> {
}
const stringBox1: Box1<string> = new Box1(); // NOK
const stringBox2: Box1 = new Box1<string>(); // OK
const stringBox3: Box1<string> = new Box1<string>(); // OK
const stringBox4 = new Box1(); // NOK

let de: Deque = new Deque(); //error
function newDeque():Deque{
  return new Deque(); //error
}
class D{
  dd: Deque;
  constructor(){
    new Deque(); //error
  }

  initDD(){
    dd = new Deque<number>();
  }
}

class Base<T>{}
class NewClass extends Base<number>{}
export let obj = new NewClass();
class NewClass2 extends Base<D>{}
export let obj2 = new NewClass2();

let test_arrayList_e: ArrayList<string> = new ArrayList(); //
let test_deque_e: Deque<String> = new Deque(); //
let test_hashMap_e: HashMap<String, Number> = new HashMap(); //
let test_hashSet_e: HashSet<String> = new HashSet(); //
let test_linkedList_e: LinkedList<String> = new LinkedList(); //
let test_plainArray_e: PlainArray<String> = new PlainArray(); //
let test_queue_e: Queue<String> = new Queue(); //
let test_treeMap_e: TreeMap<String, Number> = new TreeMap(); //
let test_treeSet_e: TreeSet<String> = new TreeSet(); //
let test_treeSet_set_e: TreeSet<TreeSet<String>> = new TreeSet();
let listenerList: LinkedList<number> = new LinkedList();
