/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
  let Int_a: FixedArray<FixedArray<Int>> = [[new Int(1)],[new Int(1)],[new Int(1)]];
  let Int_a2: FixedArray<FixedArray<Int>> = Int_a as FixedArray<FixedArray<Int>>;
  let Object_a: FixedArray<FixedArray<Object>> = Int_a as FixedArray<FixedArray<Object>>;
  let Int_a3: FixedArray<FixedArray<Int>> = Object_a as FixedArray<FixedArray<Int>>;

  let Object_: Object = Int_a as Object;
  let Int_a4: FixedArray<FixedArray<Int>> = Object_ as FixedArray<FixedArray<Int>>;

  let Long_a: FixedArray<FixedArray<Long>> = /* @@ label */Int_a as FixedArray<FixedArray<Long>>;
}


/* @@? 18:45 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@? 19:50 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@? 22:25 Warning Warning W670214: This cast can be excessive because it always succeeds */
