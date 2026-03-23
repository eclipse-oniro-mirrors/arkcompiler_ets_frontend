/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with License.
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
    private value: number = 10

    static getValue(c: B): number {
        return c.value
    }

    static setValue(c: B, v: number): void {
        c.value = v
    }
}

class B extends A {}

function main(): int {
    const b = new B()
    const result = A.getValue(b)
    A.setValue(b, 20)
    return Double.toInt(result + A.getValue(b))
}