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
 
function main(){
	test();
}

async function test() {
  let p = Promise.resolve(6);
  arktest.assertEQ(await p, 6);
  arktest.assertEQ(await p * 2, 12);
  arktest.assertEQ(await p / 2, 3);
  arktest.assertEQ(await p % 3, 0);
  arktest.assertEQ(await p + 1, 7);
  arktest.assertEQ(await p - 2, 4);
  arktest.assertEQ(await p === 6, true);
  arktest.assertEQ(await p != 4, true);
  arktest.assertEQ(1 << await p, 64);
  arktest.assertEQ(await p > 3, true);
  arktest.assertEQ(await p < 3, false);
  arktest.assertEQ(await p == 6, true);
  arktest.assertEQ(await p != 1, true);
  arktest.assertEQ(await p & 3, 2);
  arktest.assertEQ(await p ^ 2, 4);
  arktest.assertEQ(await p | 1, 7);

  let b = Promise.resolve(false);
  arktest.assertEQ(await b || true, true);
  let n = Promise.resolve(null);
  arktest.assertEQ(await n ?? 42, 42);
  let r = await p > 3 ? "yes" : "no";
  arktest.assertEQ(r, "yes");
  let x = await p;
  arktest.assertEQ(x, 6);
}