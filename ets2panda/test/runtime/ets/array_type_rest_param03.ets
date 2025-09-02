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

class A{
    name:string = ""
    age:int

    constructor(name:string, age:int){
        this.name = name
        this.age = age
    }
}

function foo(...args:Array<A>):int{
    let sum = 0;
    for(let a of args) {
        sum+=a.age
    }
    return sum
}

function main() : void {
    let a = new A("a" ,1)
    let b = new A("b" ,2)
    let c = new A("a" ,3)
    let d = new A("b" ,4)
    arktest.assertEQ(foo(a,b,c,d),10)
}
