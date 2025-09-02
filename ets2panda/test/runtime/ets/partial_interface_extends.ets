/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface I {
  i: number;
}

interface I2 extends I {
  j: string;
}

function main(){
  let f: Partial<I2> = {}
  arktest.assertEQ(f.i, undefined)
  arktest.assertEQ(f.j, undefined)

  let f2: Partial<I2> = {i:10,j :"some str" }
  arktest.assertEQ(f2.i, 10)
  arktest.assertEQ(f2.j, "some str")
}
