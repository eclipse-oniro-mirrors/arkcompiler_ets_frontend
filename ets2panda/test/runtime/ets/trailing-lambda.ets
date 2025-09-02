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

// test transformation of trailing lambda
let x: int = 1;
const y: int = 3;

function f1(callback: ()=>void) {
    callback();
}

function f2() {
    x = x + 1
}

function f3(callback: ()=>void) :int {
    callback();
    return y;
}

function test_transform() {
    x = 1
    f1() {  // Test '{' at the same line as call expression, block statement can be transformed to trailing lambda
        x++
    }
    arktest.assertEQ(x, 2, "expected: " + 2 + " actual: " + x)

    f1()
    {  // Test '{' not the same line as call expression, block statement can be transformed to trailing lambda
        x++
    }
    arktest.assertEQ(x, 3, "expected: " + 3 + " actual: " + x)

    f2()
    {  // Test this block run as a standalone code block
        x++
    }
    arktest.assertEQ(x, 5, "expected: " + 5 + " actual: " + x)

    let b = f3()
    { x++ }
    arktest.assertEQ(x, 6, "expected: " + 6 + " actual: " + x)
    arktest.assertEQ(b, y, "expected: " + y + " actual: " + b)

    let a = f3() { x++ }
    arktest.assertEQ(x, 7, "expected: " + 7 + " actual: " + x)
    arktest.assertEQ(a, y, "expected: " + y + " actual: " + a)

    arktest.assertEQ(y, f3(){})
    arktest.assertEQ(y, f3(()=>{}))
    {
    }
}

// test signature matches of trailing lambda
function f_overload(num: Int, callback: ()=>void): Int {
    return 0;
}

function f_overload(num: Int): Int {
    return num;
}

function test_overload() {
    let num = f_overload(1)
    {
        // This block will be transform to a trailing lambda
    }
    arktest.assertEQ(num, 0, "expected: " + 0 + " actual: " + num)
}

// test class methods using trailing lambda
class A {
    set_n(callback: ()=>void) {
        callback();
    }

    public static set_n_static(callback: ()=>void): void {
        callback();
    }

    static n: int = 1;
}

function test_class_method() {
    let a = new A();
    A.n = 1;
    a.set_n() {
        A.n++;
    }
    arktest.assertEQ(A.n, 2, "expected: " + 2 + " actual: " + A.n)

    A.set_n_static() {
        A.n++;
    }
    arktest.assertEQ(A.n, 3, "expected: " + 3 + " actual: " + A.n)
}

// test scenarios involving scope
function foo(c: () => void): void {
    c();
}

function foo2(a: int) : int {
    a++;
    return a;
}

class B {
    constructor (b: int) {
        this.b = b;
    }

    get_b(): int {
        return this.b
    }

    b: int = 0;
}

function test_scope(): void {
    foo() {
       let num: int = 3;
       arktest.assertEQ(num, 3, "expected: " + 3 + " actual: " + num)
       {
            num++;
            arktest.assertEQ(num, 4, "expected: " + 4 + " actual: " + num)
            {
                num++;
                arktest.assertEQ(num, 5, "expected: " + 5 + " actual: " + num)
            }
       }

       let a = foo2(num);
       arktest.assertEQ(a, 6, "expected: " + 6 + " actual: " + a)

       let b = new B(num);
       arktest.assertEQ(b.get_b(), 5, "expected: " + 5 + " actual: " + b.get_b())

       foo() {
          let k = 1
          arktest.assertEQ(k, 1, "expected: " + 1 + " actual: " + k)
       }
    };
}

// test recovery of trailing block
function test_recover_trailing_block() {
    let a = 100;
    foo(()=>{})
    {
        let k = a;
        let b = new B(k);
        arktest.assertEQ(b.get_b(), k, "expected: " + k + " actual: " + b.get_b())

        a++;
    };
    arktest.assertEQ(a, 101, "expected: " + 101 + " actual: " + a)
}

function main() {
    test_transform();
    test_recover_trailing_block();
    test_overload();
    test_class_method();
    test_scope();
}
