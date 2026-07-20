/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function main(): void {
  let t: [number, string] = [1, 'a'];
  let x: Tuple = t;

  arktest.assertTrue(x instanceof Tuple);
  arktest.assertTrue(x instanceof Tuple1)
  arktest.assertTrue(x instanceof Tuple2)
  arktest.assertFalse(x instanceof Tuple3)
  arktest.assertFalse(x instanceof Tuple5)
  arktest.assertTrue(x instanceof Object)

  arktest.assertTrue(t instanceof Tuple)
  arktest.assertTrue(t instanceof Tuple1)
  arktest.assertTrue(t instanceof Tuple2)
  arktest.assertFalse(t instanceof Tuple3)
  arktest.assertFalse(t instanceof Tuple5)
  arktest.assertTrue(t instanceof Object)
}
