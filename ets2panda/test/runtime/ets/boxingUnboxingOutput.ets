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

function main() {
  arktest.assertEQ(`${new Float(Float.MIN_VALUE)}`, `${Float.MIN_VALUE}`)
  arktest.assertEQ(`${new Float(Float.MAX_VALUE)}`, `${Float.MAX_VALUE}`)
  arktest.assertEQ(`${new Number(Number.MIN_VALUE)}`, `${Number.MIN_VALUE}`)
  arktest.assertEQ(`${new Number(Number.MAX_VALUE)}`, `${Number.MAX_VALUE}`)
}