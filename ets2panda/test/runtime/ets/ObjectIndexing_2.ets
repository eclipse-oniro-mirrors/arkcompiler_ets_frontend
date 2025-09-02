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
  private y: string[] = ["one", "two", "three"];

  $_get(ind: int) : int {
     return this.x[ind];
  }

  $_get(ind: string) : string {
    if (ind == "1") return this.y[0];
    if (ind == "2") return this.y[1];
    if (ind == "3") return this.y[2];
    return "none";
  }

  $_set(ind: int, val: int): void  {
    this.x[ind] = val;
  }

  $_set(ind: string, val: string): void  {
    if (ind == "1") this.y[0] = val;
    if (ind == "2") this.y[1] = val;
    if (ind == "3") this.y[2] = val;
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

  arktest.assertEQ(c['1'], "one")
  arktest.assertEQ(c['2'], "two")
  arktest.assertEQ(c["3"], 'three')
  arktest.assertEQ(c['7'], "none")
  arktest.assertEQ(c[""], "none")

  c['1'] = "один";
  c["2"] = c["3"];
  c["3"] += '|три';

  arktest.assertEQ(c['1'], 'один')
  arktest.assertEQ(c["2"], 'three')
  arktest.assertEQ(c['3'], "three|три")
  arktest.assertEQ(c['7'], "none")
  arktest.assertEQ(c[""], "none")
  arktest.assertEQ(c['1'] + c[1], "один5")
}
