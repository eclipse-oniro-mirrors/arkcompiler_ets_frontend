/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function main(): void {
    let intType: TypeGetter<Int> = new TypeGetter<Int>();
    let testInt: Int = 2;
    let testDouble: Double = 3.0;
    let doubleType: TypeGetter<Double> = new TypeGetter<Double>();
    intType.SetType(testInt);
    doubleType.SetType(testDouble);
    arktest.assertEQ(intType.GetType(), 2)
    arktest.assertEQ(doubleType.GetType(), 3.0)

    let aType: TypeGetter<A> = new TypeGetter<A>();
    let aClass: A = new A();
    aType.SetType(aClass);
    arktest.assertEQ(aType.GetType().aValue(), 1)

    let ext1Class: Ext1 = new Ext1();
    let ext2Class: Ext2 = new Ext2();
    arktest.assertEQ(bar(ext1Class, ext2Class), 7)
    arktest.assertEQ(bar(ext2Class, ext1Class), -7)

    let bc: B = new B();
    let cc : C = new C();
    let dc: D = new D();
    arktest.assertEQ(bc.bMet(cc).cMet(), 17)
    arktest.assertEQ(bc.bMet(cc).ifaceMet(), 10)
    arktest.assertEQ(bc.bMet(dc).dMet(), 8)
}

interface ifaceExt {
  ifmet(other: Object): int;
}

function ifaceType<T extends ifaceExt>(x: T, y: T): int {
  return x.ifmet(y);
}

class TypeGetter<TT> {
    tVar: TT;

    GetType(): TT{
        return this.tVar;
    }

    SetType(a0: TT): void{
        this.tVar = a0;
    }
}

class A {
    aValue(): int {
        return 1;
    }
}

class AConstraint<T extends A>{
    cVar: T;

    getVal(): void {
        this.cVar.aValue();
    }
}

interface iface2<T>{
    ifaceMet(): int;
}

class C implements iface2<Object>{
    public override ifaceMet(): int{
        return 10;
    }
    public cMet(): int {
        return 17;
    }
}

class D extends C{
    public dMet(): int {
        return 8;
    }
}

class B {
    public bMet<LU extends C>(a: LU): LU {
        let b: int = 2;
        a.ifaceMet();
        return a;
    }
}

interface ifaceExt2 {
  imet1(other: Object): int;
  imet2(): int;
}

function bar<T extends impExt>(x: T, y: T): int {
  let a = x.imet2();
  let b = y.imet2();
  return a - b;
}

class impExt implements ifaceExt2{
    override imet1(other: Object): int{
        return 20;
    }
    override imet2(): int{
        return 21;
    }
}

class Ext1 extends impExt{
    override imet1(other: Object): int{
        return 11;
    }
    override imet2(): int {
        return 14;
    }
}

class Ext2 extends impExt{
    override imet1(other: Object): int{
        return 22;
    }
    override imet2(): int {
        return 7;
    }
}

interface inter3<T>{
    met1(a0: T): int;
    met2(a1: T): T;
    met3(a3: T): inter3<T>;
}

class ImplInter3<T> implements inter3<Int>{
    public override met1(a0: Int): int {
        return 1;
    }

    public override met2(a0: Int): Int {
        return 2;
    }

    public override met3(a0: Int): inter3<Int> {
        let a: inter3<Int> = new ImplInter3<Int>();
        return a;
    }
}
