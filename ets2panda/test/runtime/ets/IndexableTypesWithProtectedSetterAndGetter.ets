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

class A {
    ind: string = '';
    val: number;

    protected $_set(i: string, v: number) {
        this.ind = i;
        this.val = v;
    }

    protected $_get(i: string): number {
        return i == this.ind ? this.val : Number.NaN;
    }
}

class B extends A {
    push(ind: string, val: number) {
       super[ind] = val;
    }

    pull(ind: string): number {
        return super[ind];
    }

}

function main(): int {
    let b = new B;
    b.push('a', 11.0);
    arktest.assertEQ(b.pull('a'), 11.0)
    return 0;
}