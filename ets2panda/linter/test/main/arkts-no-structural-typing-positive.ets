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

// P1: Exact Same Type (Class)
class A1 {
  v: number = 0
}
let a1: A1 = new A1(); // Should pass: Type A1 assigned to variable A1

// P2: Exact Same Type (Interface)
interface I2 {
  v: number
}
let x2: I2 = { v: 1 };
let y2: I2 = x2; // Should pass: Interface I2 assigned to variable I2

// P3: Simple Assignment without Coercion
class A3 {
  v: number = 0
}
let a3 = new A3();
let b3: A3 = a3; // Should pass: Implicit type A3 assigned to explicit type A3

// P4: Correct Function Return Type
class A4 {
  v: number = 0
}
const func4: () => A4 = () => {
  return new A4(); // Should pass: Returns the exact expected class type A4
}

// P5: Exact Generic Instantiation
class C5<T> {
  
}
class A5 {
  v: number = 0
}
let b5: C5<A5> = new C5<A5>(); // Should pass: Generic types match exactly: C5<A5> = C5<A5>