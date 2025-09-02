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

class cls {
  lambda_field: (() => int) | undefined;
  readonly num: int; // don't remove it, it is necessary to trigger an issue with the lambda
}

function main(): void {
  let c = new cls();
  c.lambda_field = (() => new Int(42)) as () => Int; // #15577
  arktest.assertEQ(c.lambda_field!(), 42)
  arktest.assertEQ(c.num, 0)
}
