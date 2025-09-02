/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class A {
  name = c'A'
}
class B extends A {
  name = c'B'
}
class C extends B {
  name = c'C';
}

function clear_accumlator_type(As: FixedArray<A>): FixedArray<A> {
  return As;
}

function main() {
  {
    let As: FixedArray<A> = new C[1];
    As[0] = new C();

    As = clear_accumlator_type(As); // workaround for:
    // Verifier warning 22: Redundant check cast
    // Accumulator type 'C[]' is always a subtype of 'FixedArray<B>'. Checkcast is redundant here.
    // It may be a sign of possible error here.
    let Bs: FixedArray<B> = As as FixedArray<B>;
    arktest.assertEQ(Bs[0].name, c'B')
  }

  {
    let caught = false;
    try {
      let As: FixedArray<A> = new A[1];
      let Bs = As as FixedArray<B>;
    } catch (e: ClassCastError) {
      caught = true;
    } catch (e) {
      arktest.assertTrue(false)
    }
    arktest.assertTrue(caught)
  }
}
