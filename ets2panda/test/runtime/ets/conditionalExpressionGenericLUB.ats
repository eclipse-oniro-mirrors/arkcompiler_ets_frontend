/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class A {}

class B extends A {}

class C extends B {}

class D extends C {}

class F extends B {}

class GenA<K> {}

class GenB<K> extends GenA<F> {}

final class GenC<K, M> extends GenB<M> {}

final class GenD<K, M> extends GenB<K> {}

function foo(p: Object): int {
  return 0;
}

function foo(p: GenA<A>): int {
  return 1;
}

function foo(p: GenA<B>): int {
  return 2;
}

function foo(p: GenB<A>): int {
  return 3;
}

function foo(p: GenB<B>): int {
  return 4;
}

function foo(p: GenB<C>): int {
  return 5;
}

function foo(p: GenB<F>): int {
  return 6;
}

function foo(p: GenC<A, A>): int {
  return 7;
}

function foo(p: GenC<B, B>): int {
  return 8;
}

function foo(p: GenC<C, F>): int {
  return 9;
}

function foo(p: GenC<F, C>): int {
  return 10;
}

function main(): void {
  let a = true ? new GenB<C> : new GenB<F>;
  let test_a : GenB<B> = a;

  arktest.assertEQ((foo(a), foo(new GenB<B>))

  let b = true ? new GenD<F, C> : new GenC<F, C>;
  let test_b : GenB<in B> = b;

  arktest.assertEQ((foo(b), 0)
  // foo(b) would call signature 'foo(p: GenB<in B>)',
  // but it's not declared (erased signature would
  // clash with other foo(p: GenB<*>) signatures),
  // so it's calling 'foo(p: Object)', which returns 0

  let c = true ? new GenC<F, C> : new GenC<C, F>;
  let test_c : GenC<B, B> = c;

  arktest.assertEQ((foo(c), foo(new GenC<B, B>))

  let d = true ? new GenA<D> : new GenB<A>;
  let test_d : GenA<B> = d;

  arktest.assertEQ((foo(d), foo(new GenA<B>))

  let f = true ? new GenB<C> : new GenB<C>;
  let test_f : GenB<C> = f;

  arktest.assertEQ((foo(f), foo(new GenB<C>))
}
