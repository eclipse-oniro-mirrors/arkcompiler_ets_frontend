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

function func<T>(x: T, key?: (a: T) => int) {
    let a = () => {
        return key ? key!(x): 0;
    }
    return a();
}

function callback(cb: () => void) {
    cb();
}

class A<T> {
    value?: T;

    update(val?: T): void {
        if (val === undefined) return;
        const updateValue = val
        callback(() => {
            const a = updateValue;
            this.value = a
        });
    }
}

function main() {
    let x = (a: int) => { return a + 1; };
    arktest.assertEQ(func<int>(10, x), 11)
    arktest.assertEQ(func<int>(10, undefined), 0)

    let a = new A<int>;
    a.update(undefined);
    arktest.assertTrue(a.value === undefined)
    a.update(48);
    arktest.assertEQ(a.value, 48)
}
