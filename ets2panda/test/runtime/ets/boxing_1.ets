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

let global_Int_: Int = 2_000_000;
let global_Object_Int_: Object = 2_000_001;
const const_global_Int_: Int = 2_000_002;
const const_global_Object_Int_: Object = 2_000_003;

class A {
  public static Int_: Int = 2_000_004;
  public static Object_Int_: Object = 2_000_005;
  public static readonly readonly_Int_: Int = 2_000_006;
  public static readonly readonly_Object_Int_: Object = 2_000_007;
}

function main(): void {
  arktest.assertEQ(global_Int_.toInt(), 2_000_000)
  arktest.assertEQ((global_Object_Int_ as Int).toInt(), 2_000_001)
  arktest.assertEQ(const_global_Int_.toInt(), 2_000_002)
  arktest.assertEQ((const_global_Object_Int_ as Int).toInt(), 2_000_003)

  arktest.assertEQ(A.Int_.toInt(), 2_000_004)
  arktest.assertEQ((A.Object_Int_ as Int).toInt(), 2_000_005)
  arktest.assertEQ(A.readonly_Int_.toInt(), 2_000_006)
  arktest.assertEQ((A.readonly_Object_Int_ as Int).toInt(), 2_000_007)
}
