/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use static'

import { Animal, Dog } from './BaseClass';

export let array1 = [1, 2, 3];
export let array2 = new Array([4, 5, 6]);
export let array3 = Array.from('hello');

export function foo1(): number[] {
  return [1, 2, 3];
}

export let animals: Animal[] = [new Dog('dog1'), new Dog('dog2')];

export let str1 = 'hello';

export let set1 = new Set<string>(['a', 'b', 'c']);

export function foo2(): Set<string> {
  return new Set<['a', 'b', 'c']>;
}

export let map1 = new Map<number, string>([[1, 'a'], [2, 'b']]);

export function foo3(): Map<number, string> {
  return new Map<number, string>([[1, 'a'], [2, 'b']]);
}

export let map4 = new Map<number, string>([[3, 'c'], [4, 'd']]);

export function getArray(domain?: string[]): string[] {
  if (domain === undefined) {
    return [];
  }
  return Array.from(domain);
}