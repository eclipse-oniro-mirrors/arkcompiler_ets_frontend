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

// Example 1
function foo(): void { }
function bar(): void { }
function foo1(): void {
  return foo();
}

//Example 2
let test5 = '1';
test5 === '1' ? foo() : bar();

//Example 3
interface BT {
  qaq: Function;
}

class C {
  private cc?: BT;

  private d(): void {
    this.cc = {
      qaq: (caller?: string): void => this.qaqq(caller)
    }
  }
  private qaqq(caller?: string): void {
    return;
  }
}

//Example 4
function foocfe(a: number): string | undefined {
  if (a >= 0) {
    return "a >= 0";
  }
  return undefined;
}

//Example 5
function roo(): number { return 3 }
let t: undefined | number = roo()