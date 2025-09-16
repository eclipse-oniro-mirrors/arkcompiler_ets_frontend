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

class X {}

async function foo(): Promise<boolean>{
  return new Promise<boolean>(() => { }); // No error in ArkTS 1.2
}

async function bar(): Promise<X>{
  return new X(); // No error in ArkTS 1.2
}

class PromiseTest {
  protected async foo(): Promise<X[]> {
    let arr: X[] = [];
    return arr;
  }

  public async bar(): Promise<X[]> {
    return this.foo(); // No error in ArkTS 1.2
  }
}

function sleep<T = void>(ms: number): PromiseLike<T> {
  return new Promise<T>(
    (resolve: (value: T | PromiseLike<T>) => void): number => setTimeout(resolve, ms)
  );
}