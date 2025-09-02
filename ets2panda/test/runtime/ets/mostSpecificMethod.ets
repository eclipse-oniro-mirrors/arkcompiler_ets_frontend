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

class KlassA {
    public Foo() : String {
        return "KlassA::Foo()";
    }

    public Bar() : String {
        return "KlassA::Bar()";
    }

    // NOTE(vpukhov): Baz order affects override resoluition!
    public Baz(x: A, y: B) : String {
        return "KlassA::Baz(B, B)";
    }

    public final Baz(x: A, y: A) : String {
        return "KlassA::Baz(A, A)";
    }
}

class KlassB extends KlassA {
    public override Foo() : String {
        return "KlassB::Foo()";
    }

    public override Bar() : String {
        return "KlassB::Bar()";
    }

    public override Baz(x: A, y: B) : String {
        return "KlassB::Baz(A, B)";
    }
}

class KlassC extends KlassB {
    public override Foo() : String {
        return "KlassC::Foo()";
    }

    public Baz(x: B, y: B) : String {
        return "KlassC::Baz(B, B)";
    }

    public final CheckThisAndSuper() : void {
        arktest.assertEQ(this.Foo(), "KlassC::Foo()");
        arktest.assertEQ(super.Foo(), "KlassB::Foo()");
    }
}

function Foo(x: A, y: B) : String { return "GLOBAL::Foo(A, B)"; }
function Foo(x: B, y: B) : String { return "GLOBAL::Foo(B, B)"; }

function Bar(x: int) : String { return "GLOBAL::Bar(int)"; }
function Bar(x: short) : String { return "GLOBAL::Bar(short)"; }
function Bar(x: char) : String { return "GLOBAL::Bar(char)"; }
function Bar(x: double) : String { return "GLOBAL::Bar(double)"; }

function Bar2(x: long) : String { return "GLOBAL::Bar2(long)"; }
function Bar2(x: double) : String { return "GLOBAL::Bar2(double)"; }

function Baz(x: Object) : String { return "GLOBAL::Baz(Object)"; }
function Baz(x: Int) : String { return "GLOBAL::Baz(Int)"; }
function Baz(x: Short) : String { return "GLOBAL::Baz(Short)"; }
function Baz(x: Char) : String { return "GLOBAL::Baz(Char)"; }
function Baz(x: String) : String { return "GLOBAL::Baz(String)"; }

function main() : void {
    let a: KlassA = new KlassA();
    let b: KlassB = new KlassB();
    let c: KlassC = new KlassC();

    arktest.assertEQ(a.Foo(), "KlassA::Foo()");
    arktest.assertEQ(b.Foo(), "KlassB::Foo()");
    arktest.assertEQ(c.Foo(), "KlassC::Foo()");

    let d: KlassA = new KlassC();
    let f: KlassB = new KlassC();

    arktest.assertEQ(d.Foo(), "KlassC::Foo()");
    arktest.assertEQ(f.Foo(), "KlassC::Foo()");
    arktest.assertEQ(c.Bar(), "KlassB::Bar()");

    let objA : A = new A();
    let objB : B = new B();

    arktest.assertEQ(c.Baz(objB, objB), "KlassC::Baz(B, B)");
    arktest.assertEQ(c.Baz(objA, objB), "KlassB::Baz(A, B)");
    arktest.assertEQ(c.Baz(objA, objA), "KlassA::Baz(A, A)");

    c.CheckThisAndSuper();

    arktest.assertEQ(Foo(objA, objB), "GLOBAL::Foo(A, B)");
    arktest.assertEQ(Foo(objB, objB), "GLOBAL::Foo(B, B)");

    arktest.assertEQ(Baz(new Int(1)), "GLOBAL::Baz(Int)");
    arktest.assertEQ(Baz(new Char(c'1')), "GLOBAL::Baz(Char)");
    arktest.assertEQ(Baz(new Short(1 as short)), "GLOBAL::Baz(Short)");
    arktest.assertEQ(Baz("hello"), "GLOBAL::Baz(String)");

    arktest.assertEQ(Bar(1), "GLOBAL::Bar(int)");
    arktest.assertEQ(Bar(c'1'), "GLOBAL::Bar(char)");
    arktest.assertEQ(Bar(3.14), "GLOBAL::Bar(double)");

    arktest.assertEQ(Bar2(1), "GLOBAL::Bar2(long)");
    arktest.assertEQ(Bar2(1 as long), "GLOBAL::Bar2(long)");
    arktest.assertEQ(Bar2(1.1), "GLOBAL::Bar2(double)");

}
