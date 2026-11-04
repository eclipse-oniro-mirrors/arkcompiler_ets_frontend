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

function foo<T>(first: () => T): T {
    const instance = first()
    return instance
}

class X {
    static foo<T>(first: () => T, second: () => void): T {
        const instance = first()
        second()
        return instance
    }

    static $_invoke<T>(first: () => T, second: () => void): T {
        const instance = first()
        second()
        return instance
    }
}

function main() {
    foo((): String => { return "XXX" });

    foo((): Int => { return new Int(42) });

    X(
    (): String => { return "XXX" }
    ) {
    }
}