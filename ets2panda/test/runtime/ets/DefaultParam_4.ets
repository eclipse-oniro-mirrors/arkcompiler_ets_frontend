/*
 * Copyright (c) 2023 - 2024-2025 Huawei Device Co., Ltd.
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

function main():void{
    let boo = new bar()

    arktest.assertEQ(boo.foo1(), 10)
    arktest.assertEQ(boo.foo2(), "a")
    arktest.assertEQ(boo.foo3(), true)

    arktest.assertEQ(boo.foo4(), 30)
    arktest.assertEQ(boo.foo4(new Int(5)), 25)

    arktest.assertEQ(boo.foo5(new Int(5)), 55)
    arktest.assertEQ(boo.foo5(new Int(5),new Int(10)), 45)


    arktest.assertEQ(boo.foo6(), undefined)
    arktest.assertEQ(boo.foo7(), undefined)
    arktest.assertEQ(boo.foo8(), undefined)

    arktest.assertEQ(boo.foo9(), 0)
    arktest.assertEQ(boo.foo9(new Int(5)), -1)

    arktest.assertEQ(boo.foo10(new Int(5)), 0)
    arktest.assertEQ(boo.foo10(new Int(5),new Int(10)), -1)

    arktest.assertEQ(boo.foo11(), 0)
    arktest.assertEQ(boo.foo12(), 0)

    arktest.assertEQ(boo.foo13(new Int(10)), 15)
    arktest.assertEQ(boo.foo14(new Int(10)), 25)

    arktest.assertEQ(boo.foo15(new Int(10),new Int(5)), 20)
    arktest.assertEQ(boo.foo16(new Int(10),new Int(5)), 30)
}

class bar {
    foo1(a : Int = 10) : Int {
        return a;
    }

    foo2(a : string = "a") : string {
        return a;
    }

    foo3(a : Boolean = true) : Boolean {
        return a;
    }

    foo4(a : Int = 10, b : Int = 20) : Int {
        return a + b;
    }

    foo5(a : Int = 10, b : Int = 20, c : Int = 30) : Int {
        arktest.assertEQ(a, 5)

        return a + b + c;
    }

    foo6(a? : Int) : Int|undefined {
        return a;
    }

    foo7(a? : string) : string|undefined {
        return a;
    }

    foo8(a? : Boolean) : Boolean|undefined {
        return a;
    }

    foo9(a? : Int, b? : Int) : Int {
        if(a == undefined && b == undefined){
            return 0;
        }
        if(b == undefined){
            return -1;
        }
        return a! + b;
    }

    foo10(a? : Int, b? : Int, c? : Int) : Int {
        arktest.assertEQ(a!, 5)
        if(b == undefined && c == undefined){
            return 0;
        }
        if(c == undefined){
            return -1;
        }

        return a! + b! + c;
    }

    foo11(a : Int = 5, b? : Int) : Int {
        if(b == undefined){
            return 0;
        }
        return a + b;
    }

    foo12(a? : Int, b : Int = 5, c? : Int) : Int {
        arktest.assertEQ(b, 5)

        if(a == undefined && c == undefined){
            return 0;
        }

        return a! + b + c!;
    }

    foo13(a : Int, b : Int = 5) : Int {
        return a + b;
    }

    foo14(a : Int, b : Int = 5, c : Int = 10) : Int {
        return a + b + c;
    }

    foo15(a : Int, b : Int, c : Int = 5) : Int {
        return a + b + c;
    }

    foo16(a : Int, b : Int, c : Int = 10, d : Int = 5) : Int {
        return a + b + c + d;
    }
}
