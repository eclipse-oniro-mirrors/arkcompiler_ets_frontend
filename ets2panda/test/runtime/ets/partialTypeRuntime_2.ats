/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class Class1 { fld: Number = 2; }

function test_1(): void {
    let class1_partial: Partial<Class1> = {fld: 3};
    arktest.assertEQ(class1_partial.fld, 3)
    class1_partial.fld = undefined;
    arktest.assertTrue(class1_partial.fld === undefined)
}

// -----------------------------------------------

class Class2 {
    fld: Number = 2;
    foo(a0: Partial<Class2>) {
        a0.fld = undefined;
    }
}

function test_2(): void {
    let class2_original: Class2 = new Class2();
    let class2_partial: Partial<Class2> = {fld: 3};
    arktest.assertEQ(class2_partial.fld, 3)
    class2_original.foo(class2_partial);
    arktest.assertTrue(class2_partial.fld === undefined)
}

// -----------------------------------------------

class Class3 {
    mmeb: Number = 2;
    foo(a0: Partial<Class5>){
        a0.mmeb = undefined;
    }
}

class Class4 extends Class3 {}

class Class5 extends Class4 {
    mmeb: Number = 2;
    foo(a0: Partial<Class3>){
        a0.mmeb = undefined;
    }
}

function test_3(): void {
    let class3_original: Class3 = new Class3();
    let class5_original: Class3 = new Class5();
    let class3_partial: Partial<Class3> = {mmeb: 8};
    let class5_partial: Partial<Class5> = {mmeb: 10};
    class3_original.foo(class5_partial);
    class5_original.foo(class3_partial);
    arktest.assertTrue(class3_partial.mmeb === undefined)
    arktest.assertTrue(class5_partial.mmeb === undefined)
}

// -----------------------------------------------

class Class6 {
    mmeb: Number = 2;
    foo(a0: Partial<Class7>){
        if(this.mmeb == 3){
            return;
        }

        this.bar(a0);
    }
    bar(a0: Partial<Class7>){
        this.mmeb = 3;
        this.foo(a0);
    }
}

class Class7 { fld: Number = 6;}

class Class8 <T extends Class7> {
    baz(a0: Partial<Class7>){
        a0.fld = undefined;
    }
}

function test_4(): void {
    let class7_partial: Partial<Class7> = {fld: 8};
    let class6_original: Class6 = new Class6();
    class6_original.foo(class7_partial);
}

// -----------------------------------------------

class Class9 { fld: Number = 8; }

class Class10 extends Class9 { memb: Number = 9; }

function test_5(): void {
    let class10_partial: Partial<Class10> = {memb: 7, fld: 5};

    arktest.assertEQ(class10_partial.memb, 7)
    arktest.assertEQ(class10_partial.fld, 5)

    class10_partial.memb = undefined;
    class10_partial.fld = undefined;

    arktest.assertTrue(class10_partial.memb === undefined)
    arktest.assertTrue(class10_partial.fld === undefined)
}

//---------------------------------------

function main(): void {
    test_1();
    test_2();
    test_3();
    test_4();
    test_5();
}
