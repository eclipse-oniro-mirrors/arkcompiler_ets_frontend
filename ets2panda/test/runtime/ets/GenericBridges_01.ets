/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

interface B<T> {
  f2(v: B<T>): B<T>; 
}

class C<T extends Numeric|string> implements B<T> {
  f1(v: T): T { return v; }

  f2(v: C<T>): C<T> { return new C<T>(); }
  f2(v: B<T>): B<T> {
     return this.f2(v as C<T>);
  }

  f5(x: T, y: T): string {return "C.f5"; }

  f6(x: C<T>, y: C<T>): string { return "C.f6"; }
  f7(x: T, y: C<T>): string { return "C.f7"; }
}

class D extends C<string> {
  f1(v: string): string { return "oh"; }
  f1(v: Int): Int { return 7; }

  f2(v: D): D { return new D(); }

  f3(){}
  f4 (x: int, y: int): int { return x + y; }

  f6(): string { return "D.f6"; }
  f7(x: string, y: D): string { return "D.f7"; }
}

class F extends D {}

class G<U extends Numeric|string> extends C<U> {}

class E<U extends Integral> extends C<U> {
  f1(v: U): Integral { 
    if (v instanceof Int) {
      return new Int(7);
    } else if (v instanceof Long) {
      return new Long(8);
    } else {
      return new Int(-1);
    }
  }

  f2(v: E<U>): E<U> { return new E<U>(); }

  f3(){}
  f4(x:int, y: int): int { return x + y; }
}


function foo1(c: C<Int>) {
 arktest.assert (c.f1(0) == 7);
 arktest.assert (c.f2(c).f1(0) == 7);
 arktest.assert (c.f5(1, 2) == "C.f5"); 
 arktest.assert (c.f6(c, c) == "C.f6");
}

function foo2(c: C<Long>) {
 arktest.assert (c.f1(0) == 8);
 arktest.assert (c.f2(c).f1(0) == 8);
 arktest.assert (c.f5(3, 4) == "C.f5");
 arktest.assert (c.f7(3, c) == "C.f7");
}


function ttt(c: C<string>): void {
  arktest.assert (c.f1("ah") == "oh"); 
  arktest.assert (c.f2(c).f1("ah") == "oh");

  let a: Object = "ah";
  arktest.assert (c.f1(a) == "oh");
  arktest.assert (c.f2(c).f1(a) == "oh");

  arktest.assert (c.f5("ah", "ah") == "C.f5");
  arktest.assert (c.f6(c, c) == "C.f6");;
  arktest.assert ((c as D).f6() == "D.f6");
  arktest.assert (c.f7("ah", c) == "D.f7");
}

function main() {
  ttt(new D())
  let c: C<Int> = new E<Int>();
  foo1(c);
  foo2(new E<Long>());
}
