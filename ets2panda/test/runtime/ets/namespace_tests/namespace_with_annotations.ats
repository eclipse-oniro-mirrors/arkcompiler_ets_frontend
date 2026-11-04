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

enum Color{RED, BLUE, GREEN}

@Retention("SOURCE")
@interface Anno{}

@Retention("SOURCE")
@interface ClassAuthor {
    authorName: string = "Jim"
    authorAge: number = -35
    testBool: boolean = false
    favorColor: Color = Color.BLUE
    color: FixedArray<Color> = [Color.RED, Color.BLUE]
    reviewers: FixedArray<string> = ["Bob", "Jim", "Tom"]
    reviewersAge: FixedArray<number> = [18, 21, 32]
    testBools: FixedArray<boolean> = [false, true, false]
    mutiArray: FixedArray<FixedArray<number>> = [
        [1, 2, 3],
        [4, +5, 6],
        [7, 8, -9]
    ]
}

@Anno
@ClassAuthor
namespace A{
    @Anno
    @ClassAuthor
    let a = 1

    @Anno
    @ClassAuthor
    function foo(@Anno a:int){}

    @Anno
    @ClassAuthor
    interface MyInterface{
        @Anno
        @ClassAuthor
        value:string

        @Anno
        @ClassAuthor
        bar(){}
    }

    @Anno
    @ClassAuthor
    class A{
        @Anno
        @ClassAuthor
        b:number = 1

        @Anno
        @ClassAuthor
        foo(){}
    }
}

function main(){}