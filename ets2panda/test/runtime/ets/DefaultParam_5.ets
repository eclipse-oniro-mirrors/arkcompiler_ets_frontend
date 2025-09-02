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

function main():void{
    arktest.assertEQ(foo2(), 5)

    let bar = new Bar();
    arktest.assertEQ(bar.bar2(), 5)

    arktest.assertEQ(goo1(), 15)
    arktest.assertEQ(goo2(), 15)
    arktest.assertEQ(goo2((a0:int):int=>{return 25;}), 25)

    arktest.assertEQ(boo1(), 18)
    arktest.assertEQ(boo2(1,2,3), 6)

    arktest.assertEQ(doo1(), 8)

    too1();
    too2();
    too3();
}

function foo1(): int {
    return 5;
}

function foo2(a : int = foo1() ): int {
    return a;
}

class Bar {
    bar1(): int {
        return 5;
    }

    bar2(a : int = this.bar1() ): int {
        return a;
    }
}

function goo1(a : () => int = (): int => {return 15;}): int {
    return a();
}

function goo2(a:(a0:int) => int = (a0:int):int=>{return 15;} ): int {
    return a(5);
}

function boo1(a0:int = boo1(1,2,3),a1:int = boo1(1,2,3),a2:int = boo1(1,2,3)):int{
    return a0+a1+a2;
}

function boo2(a0:int,a1:int,a2:int = boo2(1,2,3)):int{
    return a0+a1+a2;
}

function doo1(a0:int = 5+3):int{
    return a0;
}

function too1(a0:int = 5 /*comment*/):int{
    return a0;
}

function too2(a0:int = 5 //comment
):int{
    return a0;
}

function too3(a0:int = 5 /*comment1*/, a1:int = 5 //comment2
):int{
    return a0;
}
