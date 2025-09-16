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

interface B<T> {
  f2(v: B<T>): B<T>; 
}

class C<T extends Numeric|string, Z, T1 = string> implements B<T> {

  f1(v: T): T { 
    return v; 
  }

  f2(v: C<T, Z>): C<T, Z> { 
     return new C<T, Z>(); 
  }

  f2(v: B<T>): B<T> {
     return this.f2(v as C<T, Z>);
  }

  f3(v: T|string): string {
    return "C.f3";
  }

  f4(v: C<T, Z>|Numeric): string {
    return "C.f4";
  }

  f5(x: T1|Z|C<T, Z, T1>[]): string {
    return "C.f5"; 
  }

  f6(x: C<T, Z>, y: C<T, Z>): string { 
    return "C.f6"; 
  }

  f7(x: T, y: C<T, Z>): string { 
    return "C.f7"; 
  }

  f8(x: string): string {
    return "C.f8";
  }

  f9(z: Z, y: T1): string {
    return "C.f9";
  }
}

class D<W> extends C<string, W> {
  f1(v: string): string { 
    return "D.f1"; 
  }
  
  f1(v: Numeric|string|C<Int, W>): string { 
    return this.f1(v as string); 
  } 

  f1(v: Int): Int { 
    return 7; 
  }

  f2(v: D<W>): D<W> { 
    return new D<W>(); 
  }

  f3(v: string): string {
    return "D.f3";
  }

  f4(v: D<W>): string {
    return "D.f4";
  }

  f4 (x: int, y: int): int { 
    return x + y; 
  }

  f5(x: string|W|C<string, W>[]): string {
    return "D.f5";
  }

  f6(): string { 
    return "D.f6"; 
  }

  f7(x: string, y: D<W>): string { 
    return "D.f7"; 
  }

  f8(x: string): string {
    return "D.f8";
  }

  f9(z: W, y: string): string {
    return "D.f9-1";
  }

  f9(z: W, y: Int): string {
    return "D.f9-2";
  }
}

class F extends D<string> {}

class G<U extends Numeric|string> extends C<U, string, Int> {}

class E<U extends Integral> extends C<U, string> {

  f1(v: U): Integral { 
    if (v instanceof Int) {
      return new Int(7);
    } else if (v instanceof Long) {
      return new Long(8);
    } else {
      return new Int(-1);
    }
  }

  f2(v: E<U>): E<U> { 
     return new E<U>(); 
  }

  f3(){}

  f4(x:int, y: int): int { return x + y; }

  f7(x: U, y: E<U>): string { 
    return "E.f7"; 
  }
}

function foo1(c: C<Int, string>) {
  arktest.assertEQ(c.f1(0), 7)
  arktest.assertEQ(c.f2(c).f1(0), 7)
  arktest.assertEQ(c.f5(""), "C.f5")
  arktest.assertEQ(c.f6(c, c), "C.f6")
}

function foo2(c: C<Long, string>) {
 arktest.assertEQ(c.f1(0), 8)
 arktest.assertEQ(c.f2(c).f1(0), 8)
 arktest.assertEQ(c.f5(""), "C.f5")
 arktest.assertEQ(c.f7(3, c), "E.f7")
}

function ttt(c: C<string, string>): void {
  arktest.assertEQ(c.f1("ah"), "D.f1")
  arktest.assertEQ(c.f2(c).f1("ah"), "D.f1")
  arktest.assertEQ((c.f2(c as B<string>) as C<string, string>).f1("ah"), "D.f1")
  arktest.assertEQ(c.f3("ah"), "D.f3")
  arktest.assertEQ(c.f4(c), "D.f4")
  arktest.assertEQ(c.f5("ah"), "D.f5")
  arktest.assertEQ(c.f6(c, c), "C.f6")
  arktest.assertEQ((c as D<string>).f6(), "D.f6")
  arktest.assertEQ(c.f7("ah", c), "D.f7")
  arktest.assertEQ(c.f8(""), "D.f8")
  arktest.assertEQ(c.f9("", ""), "C.f9")
  arktest.assertEQ((c as D<string>).f9("", 0), "D.f9-2")
}

function main() {
  ttt(new D<string>())
  let c: C<Int, string> = new E<Int>();
  foo1(c);
  foo2(new E<Long>());
}
