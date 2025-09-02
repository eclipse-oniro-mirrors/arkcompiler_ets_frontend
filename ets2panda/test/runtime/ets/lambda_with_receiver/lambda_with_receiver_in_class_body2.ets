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
	X: number;
	Y: number;
    sum: number = 0;
    constructor(x:number, y:number) {
        this.X = x;
        this.Y = y;
    }
}

class B {
    aa: A[];
    constructor() {
        this.aa = [new A(1, 2), new A(3, 4), new A(5, 6)];
    }

    apply(ff: (this: A) => number) {
        for (let a of this.aa) {
            arktest.assertEQ(a.ff(), a.sum)
            arktest.assertEQ(ff(a), a.sum)
        }
    }
}

function main(){
    new B().apply((this: A): number => { this.sum = this.X + this.Y; return this.sum; });
}