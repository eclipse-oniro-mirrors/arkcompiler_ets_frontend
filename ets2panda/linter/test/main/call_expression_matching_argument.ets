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

interface A {
    value: string;
}

interface Outer {
    inner: A
}

interface No {
    inner: string
}

class Test {
    sv1: number = 1
    sv2: string = "some Value";
    sv3: boolean = true;
    sv4: A = { value: "something" }
    sv5: Outer = { inner: sv4 }
    sv6: No = { inner: sv2 }
    sv7: Array<string> = [sv2, sv2, sv2];

    someMethod() {
        func(this.sv2) //error
        func(this.sv4) //valid
        func(this.sv6.inner) // error
        func(this.sv5.inner) //valid
        func2(this.sv2) //valid
        func2(this.sv4) //valid
        func3(this.sv2) //valid
        func4(this.sv2, this.sv1) //valid
        func5(this.sv7) //valid
    }
}


function func(param: A) {
    console.log(A);
}

function func2(param: A | any) {
    console.log(param);
}

function func3(param: string | Promise<string>) {
    console.log(param);
}

function func4(param: string, param2: any[]) {
    console.log(param, ...param2);
}

function func5(param: Array<string>) {
    param.forEach(elem => console.log(elem))
}

