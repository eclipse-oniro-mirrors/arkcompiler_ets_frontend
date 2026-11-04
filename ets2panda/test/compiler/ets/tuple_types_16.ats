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

type Func1 = (value: String) => void
type Func0 = () => void;
type FuncTuple2 = [Func1, Func1]
type FuncTuple3 = [Func1, Func1, Func0]

function main(): void {
  let t2: FuncTuple2 = [(value: String) => {}, (value: String) => {}]
  let t3: FuncTuple3 = [(value: String) => {}, (value: String) => {}, () => {}]
}
