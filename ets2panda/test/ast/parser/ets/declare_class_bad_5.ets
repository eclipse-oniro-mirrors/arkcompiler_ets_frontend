/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

declare class A {
  private constructor()
  internal constructor(x : int)
  private foo1() : int
  internal foo2() : int
  private get x1() : int
  internal get x2() : int
  private set x1(x : int)
  internal set x2(x : int)
}

declare class CIterator implements Iterator<string> {

  public ind_: int;
  public base_: C;

  constructor (base: C)

  override next(): IteratorResult<string>
}

declare class B {
  private [Symbol.iterator] () : CIterator;
}

declare class C {
  internal [Symbol.iterator] () : CIterator;
}

declare class D {
  private [index: number]: number
}

declare class E {
  internal [index: number]: number
}

/* @@? 17:3 Error Syntax error ESY0238: Methods or fields should not be decorated with private in ambient class. */
/* @@? 18:11 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 19:3 Error Syntax error ESY0238: Methods or fields should not be decorated with private in ambient class. */
/* @@? 20:3 Error Semantic error ESE0351: Variable 'internal' has already been declared. */
/* @@? 20:11 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:3 Error Syntax error ESY0238: Methods or fields should not be decorated with private in ambient class. */
/* @@? 22:3 Error Semantic error ESE0351: Variable 'internal' has already been declared. */
/* @@? 22:11 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 23:3 Error Syntax error ESY0238: Methods or fields should not be decorated with private in ambient class. */
/* @@? 24:3 Error Semantic error ESE0351: Variable 'internal' has already been declared. */
/* @@? 24:11 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 38:3 Error Syntax error ESY0238: Methods or fields should not be decorated with private in ambient class. */
/* @@? 42:11 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 46:3 Error Syntax error ESY0238: Methods or fields should not be decorated with private in ambient class. */
/* @@? 50:11 Error Syntax error ESY0121: Field type annotation expected. */