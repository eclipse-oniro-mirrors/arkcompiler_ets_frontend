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

class A {
    foo(): void{
        let a: boolean = new Boolean(true);
        let b: Boolean = true;
        let r: byte = 20;
        let c: byte = new Byte(r);
        let d: Byte = 20;
        let s: short = 2000;
        let e: short = new Short(s);
        let t: Short = 20;
        let f: Short = 2000;
        let g: char = new Char(c'a');
        let h: Char = c'a';
        let i: int = new Int(200000);
        let u: Int = 20;
        let v: Int = 2000;
        let j: Int = 200000;
        let k: long = new Long(200000000000);
        let l: Long = 200000000000;
        let m: float = 2.22f;
        let n: float = new Float(m);
        let o: Float = m;
        let p: double = new Double(2.2222222222);
        let q: Double = 2.2222222222;
    }

    booleanPrimitive(a: boolean): void {}
    booleanReference(a: Boolean): void {}
    bytePrimitive(a: byte): void {}
    byteReference(a: Byte): void {}
    shortPrimitive(a: short): void {}
    shortReference(a: Short): void {}
    charPrimitive(a: char): void {}
    charReference(a: Char): void {}
    integerPrimitive(a: int): void {}
    integerReference(a: Int): void {}
    longPrimitive(a: long): void {}
    longReference(a: Long): void {}
    floatPrimitive(a: float): void {}
    floatReference(a: Float): void {}
    doublePrimitive(a: double): void {}
    doubleReference(a: Double): void {}

    checker(): void {
        this.booleanPrimitive(new Boolean(false));
        this.booleanReference(false);
        let r: byte = 20;
        this.bytePrimitive(new Byte(r));
        this.byteReference(r);
        let s: short = 2000;
        this.shortPrimitive(new Short(s));
        this.shortReference(s);
        this.charPrimitive(new Char(c'a'));
        this.charReference(c'a');
        this.integerPrimitive(new Int(200000));
        this.integerReference(200000);
        this.longPrimitive(new Long(200000000000000));
        this.longReference(200000000000000);
        let f: float = 2.22f;
        this.floatPrimitive(new Float(f));
        this.floatReference(f);
        this.doublePrimitive(new Double(2.2222222222));
        this.doubleReference(2.2222222222);
    }
}
