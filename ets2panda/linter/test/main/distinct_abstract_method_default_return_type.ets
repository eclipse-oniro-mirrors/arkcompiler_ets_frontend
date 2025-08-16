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

// ───────────────────────────────────────────────────────────────────────────
// CASE 1: Simple override, no explicit type, non-async → OK
// Base has abstract foo() (no annotation), override without annotation defaults to void
abstract class A1 {
  abstract foo();
}
class B1 extends A1 {
  foo() {          // ✅ no error
    // …
  }
}

// ───────────────────────────────────────────────────────────────────────────
// CASE 2: Explicit void override → OK
abstract class A2 {
  abstract foo();
}
class B2 extends A2 {
  foo(): void {    // ✅ no error
    // …
  }
}

// ───────────────────────────────────────────────────────────────────────────
// CASE 3: Non-void override → ERROR
abstract class A3 {
  abstract foo();
}
class B3 extends A3 {
  foo(): number {  // ❌ should flag AbstractOverrideReturnTypeNotVoid
    return 42;
  }
}

// ───────────────────────────────────────────────────────────────────────────
// CASE 4: Async override without annotation → ERROR
// async methods default to Promise<void>, and base foo() had no type
abstract class A4 {
  abstract foo();
}
class B4 extends A4 {
  async foo() {    // ❌ should flag AbstractOverrideReturnTypeNotVoid
    await Promise.resolve();
  }
}

// ───────────────────────────────────────────────────────────────────────────
// CASE 5: Indirect inheritance through an intermediate class
abstract class A5 {
  abstract foo();
}
class B5 extends A5 { /* no override here */ }
class C5 extends B5 {
  foo(): number {  // ❌ should flag — walks up to A5
    return 1;
  }
}

// ───────────────────────────────────────────────────────────────────────────
// CASE 6: Base declares a return type
abstract class A6 {
  abstract bar(): string;  // explicit annotation
}
class B6 extends A6 {
  bar() {                 // ✅ no error (base had annotation)
    return "ok";
  }
}
class C6 extends A6 {
  bar(): number {         // ✅ no InvalidAbstractOverrideReturnType error (this rule only applies when base had no type and derived method's type is different from void)
    return 123;
  }
}

// ───────────────────────────────────────────────────────────────────────────
// CASE 7: Class with its own non-override method — no error
class OwnClass {
  fooOwn() {     // ✅ no error
    return 'own';
  }
}
