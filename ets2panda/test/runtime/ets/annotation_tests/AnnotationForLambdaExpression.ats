/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
enum Color{RED, GREEN, BLUE}

@Retention("SOURCE")
@interface ClassAuthor{
    authorName: string = "Jim"
    authorAge: number = 35
    testBool: boolean = false
    favorColor: Color = Color.BLUE
    color: Color[] = [Color.RED, Color.BLUE]
    reviewers: string[] = ["Bob", "Jim", "Tom"]
    reviewersAge: number[] = [18, 21, 32]
    testBools: boolean[] = [false, true, false]
    mutiArray: number[][] = [
        [1, 2, 3],
        [4, -5, 6],
        [7, 8, -9]
    ]
}

@Retention("SOURCE")
@interface Log{
    msg:string = ""
}

@Retention("SOURCE")
@interface Anno{}
 
const base = @ClassAuthor (x: number) => x * 2;

const multipleAnno = @ClassAuthor @Log() @Anno (x: number) => x + 1;

const withArg = @Log("msg") (x: number) => x + 2;

const nested1 = @Log (x: number) => {
    const inner = @Anno (y: number) => y * 2;
    return inner(x) + 1;
};
const nested2 = @Log("nested") ((x: number) => {
    const inner = @Anno (y: number) => y * x;
    return inner(x);
});

const deeplyNested = @ClassAuthor() @Anno @Log() (x: number) => {
    const level1 = @ClassAuthor @Anno @Log (y: number) => {
        const level2 = @ClassAuthor() @Anno() @Log() (z: number) => z * 2;
        return level2(y) + 1;
    };
    return level1(x);
};

@ClassAuthor (x: number) => x * 2;

function foo(a:()=>number){}
 
function main(){
    foo(@Log ()=>1)
}

class A { name = "Bob" }
let a = new A();
let show = @Anno(this: A): string => {
    return "Hi," + this.name;
}

let show2 = @Anno()(this: A): string => {
    return "Hello," + this.name;
}