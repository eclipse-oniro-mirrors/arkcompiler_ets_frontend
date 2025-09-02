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

class A {}
class B extends A {}
class Gen<T> {}
type tmp = (A | null) | null | null;
type tmp2 = A | null;
type A_alias = A;
type NullableA_alias = (A_alias | null) | null;

function main(): void {
  nullableType();
  nullableTypeAlias();
  nullableLambda();
}

function nullableType(): void {
  let a : Object | null = null;
  let b : Object | null = new Object();
  let c : Object | null | null = b;
  let d : (((Object) | null) | null) | null = numericNullableType();
  let f : A | null = null;
  let g : tmp | null = f;
  let g2 : tmp2 = null;
  a = f;
  b = g;
  g = new A();

  let generic : Gen<A | null> | null = new Gen<A | null>;

  let h : Char | null = nullableArgsAndReturn(7, g);
  arktest.assertEQ(h, c'C')

  let gt1 : Gen<A | null> | null = nullableGenArgAndReturn(generic);
  arktest.assertEQ(gt1, generic)

  let gt2 = new Gen2<A_alias>();
  let gt2_g = gt2.foo(g);

  arktest.assertEQ(gt2_g, g)

  let f_g = foo_gen<tmp>(g);

  arktest.assertEQ(f_g, g)

  let arr = new long[1024];
  a = arr;
  let narr : int[] | null = null;
  a = narr;
}

function nullableArrayType(): void {
  let arron : (Object | null)[] | null = [1, 2, null];
  arron = null;
}

function nullableTypeAlias(): void {
  let o : Object;
  o = new Object();
  o = new A();
  o = new A_alias();
  o = new B();

  let on : Object | null;
  on = null;
  on = new Object();
  on = new A();
  on = new A_alias();
  on = new NullableA_alias();
  on = new B();
  on = o;

  let an : A | null;
  an = null;
  an = new A();
  an = new A_alias();
  an = new NullableA_alias();
  an = new B();
  on = an;

  let na : NullableA_alias;
  na = null;
  na = new A();
  na = new A_alias();
  na = new NullableA_alias();
  na = new B();
  na = an;
  on = na;

  let bn : B | null;
  bn = null;
  bn = new B();
  on = bn;
  an = bn;
  na = bn;
}

// example from an older version of the spec
function numericNullableType(): Object | null {
  let a: int = 5;
  let x: Int | null = 5 + a;
  arktest.assertEQ(x, 10)

  let y: Long | null = null;
  arktest.assertEQ(y, null)

  return null;
}

function nullableArgsAndReturn(arg1 : Int | null | null, args2 : (tmp | null) | null) : Char | null | null {
  return c'C';
}

function nullableGenArgAndReturn(arg : (Gen<A | null>) | null) : Gen<tmp | null> | null {
  return arg;
}

class Gen2<T extends A> {
  private t : T | null = null;
  public foo(t : T | null) : T | null {
    this.t = t;

    return this.t;
  }
}

function foo_gen<T extends A>(arg : T | null) : T | null {
  return arg;
}

function nullableLambda() : void {
  let y: int = 1;
  let x: (y : int) => Int | null = (y : int): Int | null => { return null; };
  x(y);
}

function similarParam(a : A) : int {
  return 1;
}

function similarParam(a : A | null) : int {
  return 2;
}
