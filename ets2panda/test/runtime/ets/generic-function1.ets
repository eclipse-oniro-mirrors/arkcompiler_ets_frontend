/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
    static $_instantiate<T extends A>(factory: () => T, content: () => void): T {
        const instance = factory()
        instance.__initializeStruct()
        content()
        return instance
    }
    __initializeStruct(): A {
        return this
    }
}
    
class B extends A {
    constructor() {}
    static $_instantiate<T extends B>(factory: () => T, content: () => void): T {
        return A.$_instantiate(factory, content)
    }
    override __initializeStruct(): A {
        return this
    }
}
    
export function main(): int {
    console.println("Start")

    B(){}

    return 0;
}
