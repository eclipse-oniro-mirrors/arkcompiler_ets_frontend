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

import { PersistentStorage } from './ui_modules/common_ts_ets_api';

class MyClassA {}

function MyGet1() {
  if (1 > 0) {
    return 1
  } else {
    return 2
  }
}

function MyGet2() {
  if (1 > 0) {
    return 0
  } else {
    return new MyClassA()
  }
}

const a1 = new MyClassA();
const a2 = new MyClassA();
const myArr1: Array<number> = new Array(1, 2);
const mySet1: Set<number> = new Set([1, 2, 3]);
const myMap1: Map<string, number> = new Map([["jack", 3], ["tom", 2]]);
const myMap2: Map<number, MyClassA> = new Map([[1, a1], [2, a2]]);

PersistentStorage.persistProps([{ key: 'highScore', defaultValue: '0' }, { key: 'wightScore', defaultValue: '1' }]);
PersistentStorage.persistProps([{ key: 'highScore', defaultValue: 0 }, { key: 'wightScore', defaultValue: 1 }]);
PersistentStorage.persistProps([{ key: 'highScore', defaultValue: false }, { key: 'wightScore', defaultValue: true }]);

PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new MyClassA() }]); // error
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: 1 < 0 ? 1 : new MyClassA() }]); // error
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: MyGet1() }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: MyGet2() }]); // error

PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Array(1, 2) }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Array("jack", "tom") }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Array(myArr1) }]); // error
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Array(new MyClassA()) }]); // error
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Array(a1) }]); // error

PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Set([1, 2]) }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Set(["jack", "tom"]) }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: mySet1 }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Set([new MyClassA()]) }]); // error
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Set([a1]) }]); // error

PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Map([["jack", 3], ["tom", 2]]) }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: new Map([[1, a1], [2, a2]]) }]); // error
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: myMap1 }]);
PersistentStorage.persistProps([{ key: 'PropA', defaultValue: '0' }, { key: 'PropB', defaultValue: myMap2 }]); // error