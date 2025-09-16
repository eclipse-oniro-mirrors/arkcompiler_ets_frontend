/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function createMap<K, V>(keys: FixedArray<K>, values: FixedArray<V>): Map<K, V> {
  arktest.assertEQ(keys.length, values.length)
  let map = new Map<K, V>();
  for (let i = 0; i < keys.length; i++) {
    map.set(keys[i], values[i]);
  }

  return map;
}

let map = createMap<number, string>([10, 20], ["hello", "world"]);
arktest.assertEQ(map.get(10), "hello")