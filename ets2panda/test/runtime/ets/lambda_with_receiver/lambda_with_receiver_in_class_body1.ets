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

class A {
	X: number
	Y: number
    constructor(x:number, y:number) {
        this.X = x;
        this.Y = y;
    }
}

class B {
    a: A;
    constructor(x:number, y:number) {
        this.a = new A(1, 2);
    }

    foo() {
        let getSum = (this: A): number => { return this.X + this.Y }
        arktest.assertEQ(this.a.getSum(), 3)
        arktest.assertEQ(getSum(this.a), 3)
    }
}

function main(){
    new B(1, 2).foo();
}
