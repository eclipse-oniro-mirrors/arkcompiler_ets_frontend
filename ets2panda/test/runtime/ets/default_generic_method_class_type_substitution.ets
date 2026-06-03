/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

class DefaultGenericBase {}
class DefaultGenericDerived extends DefaultGenericBase {}

class DefaultGenericHolder<T extends DefaultGenericBase> {
    value: T

    constructor(value: T) {
        this.value = value
    }

    valueOrDefault<U = T>(): U {
        return this.value as U
    }

    keep<U extends T>(value: U): U {
        return value
    }
}

function eraseDefaultGenericType(value: Object): Object {
    return value
}

function main(): int {
    let baseHolder = new DefaultGenericHolder<DefaultGenericBase>(new DefaultGenericBase())
    let baseValue: DefaultGenericBase = baseHolder.valueOrDefault()

    let derivedHolder = new DefaultGenericHolder<DefaultGenericDerived>(new DefaultGenericDerived())
    let inferredDerived: DefaultGenericDerived = derivedHolder.valueOrDefault()
    let explicitDerived: DefaultGenericDerived = derivedHolder.valueOrDefault<DefaultGenericDerived>()
    let keptDerived: DefaultGenericDerived = derivedHolder.keep<DefaultGenericDerived>(new DefaultGenericDerived())

    arktest.assertTrue(eraseDefaultGenericType(baseValue) instanceof DefaultGenericBase)
    arktest.assertTrue(eraseDefaultGenericType(inferredDerived) instanceof DefaultGenericDerived)
    arktest.assertTrue(eraseDefaultGenericType(explicitDerived) instanceof DefaultGenericDerived)
    arktest.assertTrue(eraseDefaultGenericType(keptDerived) instanceof DefaultGenericDerived)

    return 0
}
