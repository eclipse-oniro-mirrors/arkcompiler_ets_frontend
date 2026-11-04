/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class A{

  private m: int;

  foo(n: int): void {
    let x: FixedArray<int> = [1, 2, 3]
    for (/* @@ label */this.m of x) {
      console.println(n)
    }
  }
}

function main(): void {
  let n: int
  new A().foo(n)
}


/* @@@ label Error Semantic error ESE0099: Cannot obtain iterator type in 'for-of' statement. */