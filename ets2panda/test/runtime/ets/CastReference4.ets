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

interface I {}
class A implements I {}
class B implements I {}

function clear_accumlator_type(Is: FixedArray<I>): FixedArray<I> {
  return Is;
}

function main() {
  let As: FixedArray<A> = new A[1];
  let Is: FixedArray<I> = As as FixedArray<I>;

  let caught = false;
  try {
    Is = clear_accumlator_type(Is); // workaround for:
    // Verifier warning 24: Impossible array check cast
    // Element type in array in accumulator is of incompatible type 'A'.
    // Instruction 'checkcast' will always throw an exception here. It may be a sign of possible error here.
    let Bs: FixedArray<B> = Is as FixedArray<B>;
  } catch (e: ClassCastError) {
    caught = true;
  } catch (e) {
    arktest.assertTrue(false)
  }
  arktest.assertTrue(caught)

  Is = clear_accumlator_type(Is); // workaround for:
  // Verifier warning 24: Impossible array check cast
  // Element type in array in accumulator is of incompatible type 'B'.
  // Instruction 'checkcast' will always throw an exception here. It may be a sign of possible error here.
  let As2 = Is as FixedArray<A>;
}

