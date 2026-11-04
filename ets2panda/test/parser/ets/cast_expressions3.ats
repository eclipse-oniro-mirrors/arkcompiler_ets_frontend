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

class A {}
interface I {}
class B extends A implements I {}
class C extends B {}

function widening_reference_test(): void {
  let C_ = new C();

  let C_C = C_ as C;
  let C_B = C_ as B;
  let C_A = C_ as A;
  let C_I = C_ as I;

  let B_B = C_B as B;
  let B_A = C_B as A;
  let B_I = C_B as I;
}
