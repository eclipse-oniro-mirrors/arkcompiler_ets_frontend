/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

let globalVal: int = 100;

interface A {
  innerVar: int;
  successPath(): int;
  catchUsesCaptured(): int;
  catchUsesInnerVar(): int;
  catchUsesLocalAndGlobal(): int;
  catchUsesInnerAndParam(a: int): int;
  catchUsesAll(a: int): int;
  catchUsesFunction(a: int): int;
}

function bar(base: int): A {
  return {
    innerVar: 10,
    successPath(): int {
      try {
        return this.innerVar + 1;
      } catch (e) {
        console.log("Error: " + e);
        return 0;
      }
    },

    catchUsesCaptured(): int {
      try {
        throw new Error();
      } catch (e) {
        console.log("Error: " + e);
        return base;
      }
    },

    catchUsesInnerVar(): int {
      try {
        throw new Error();
      } catch (e) {
        console.log("Error: " + e);
        return this.innerVar;
      }
    },

    catchUsesLocalAndGlobal(): int {
      let local: int = 3;
      try {
        throw new Error();
      } catch (e) {
        console.log("Error: " + e);
        return local + globalVal;
      }
    },

    catchUsesInnerAndParam(a: int): int {
      try {
        throw new Error();
      } catch (e) {
        console.log("Error: " + e);
        return this.innerVar + a;
      }
    },

    catchUsesAll(a: int): int {
      let local: int = 5;
      try {
        throw new Error();
      } catch (e) {
        console.log("Error: " + e);
        return base + local + globalVal + a + this.innerVar;
      }
    },

    catchUsesFunction(a: int): int {
      let local: int = 5;
      try {
        throw new Error();
      } catch (e) {
        console.log("Error: " + e);
        return base + local + globalVal + a + this.innerVar + this.catchUsesAll(2);
      }
    }
  };
}

function main(): void {
  let value = bar(7);

  arktest.assertEQ(value.successPath(), 11);
  arktest.assertEQ(value.catchUsesCaptured(), 7);
  arktest.assertEQ(value.catchUsesInnerVar(), 10);
  arktest.assertEQ(value.catchUsesLocalAndGlobal(), 103);
  arktest.assertEQ(value.catchUsesInnerAndParam(20), 30);
  arktest.assertEQ(value.catchUsesAll(3), 125);
  arktest.assertEQ(value.catchUsesFunction(3), 249);
}