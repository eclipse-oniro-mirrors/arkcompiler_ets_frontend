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

class C {
  private x: int[] = [1, 3, 5];

  $_get(ind: int) : int {
     return this.x[ind];
  }

  $_set(ind: int, val: int): void  {
    this.x[ind] = val;
  }

}

function main(): void {

  let x: int[] = [1, 3, 5];

  let y: int = x[0];
  let z  = x[1];
  x[1] = x[2];
  x[2] *= 2;

  let c = new C();

  let u = c[0];
  let v = c.$_get(1);
  c[1] = c[2];
  c[2] *= 2;

  arktest.assertEQ(y, u)
  arktest.assertEQ(z, v)
  arktest.assertEQ(x[1], c[1])
  arktest.assertEQ(x[2], c[2])
  arktest.assertEQ(x[0], c[0])
}
