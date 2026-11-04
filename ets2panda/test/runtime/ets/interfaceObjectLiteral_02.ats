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

interface I {
  foo (i: int): string {
    let s = i.toString();
    return s;
  };

  foo (name: string): string;

  foo (): int;
}

let header = "x = ";

function main() {

  let x: int = 0;

  let p: I = {
    foo (header: string): string {
      return header + this.foo(this.foo());
    },
    foo (): int {
      return ++x;
    },

  };

  arktest.assertEQ(p.foo(header), "x = 1");
  arktest.assertEQ(p.foo(header), "x = 2");

  arktest.assertEQ(x, 2);
  ++x;

  arktest.assertEQ(p.foo(header), "x = 4");
}
