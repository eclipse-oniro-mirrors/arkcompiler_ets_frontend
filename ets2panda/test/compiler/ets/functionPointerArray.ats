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

type func_type = () => void;

function foo() : void {}

function CreateArray(a: func_type): func_type[] {
    return [a]
}

function main(): void {
  let arr: func_type[] = [foo];

  let a: func_type = foo
  let array: func_type[] = [a]

  let array_too = CreateArray(a)
}
