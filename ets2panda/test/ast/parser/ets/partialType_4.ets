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

interface CustomComponent {}
class ArkComponent {
    width(value: string): ArkComponent {
        console.log("setting width 100%")
        return this
    }
    height(value: string): ArkComponent {
        console.log("setting width 100%")
        return this
    }
    color(value: string): ArkComponent {
        console.log("setting width 100%")
        return this
    }
}

function remember<Value>(exec: () => Value): Value {
    return exec()
}

export class ArkStructCommon extends ArkComponent implements CustomComponent {}

export abstract class ArkCommonStruct0<T extends ArkCommonStruct0<T>> extends ArkStructCommon {
    static _instantiate<S extends ArkCommonStruct0<S>>(builder: ((instance: S) => void) | undefined, factory: () => S, initializers?: Partial<S>): S {
        const receiver = remember(() => {
            const instance: S = factory()
            instance.__initializeStruct(initializers)
            return instance;
        });

        receiver._buildWrapper(builder, initializers)
        return receiver
    }
    protected __initializeStruct(initializers: Partial<T> | undefined): void {}

    protected __updateStruct(initializers: Partial<T> | undefined): void { }

    _buildWrapper(builder: ((instance: T) => void) | undefined, initializers: Partial<T> | undefined): void {
            this.__updateStruct(initializers)
            this.build(builder)
    }

    build(builder: ((instance: T) => void) | undefined): void {}
}

class Column extends ArkCommonStruct0<Column> {
    build() {}
}

class ArkMyStructComponent extends ArkCommonStruct0<ArkMyStructComponent> {
    __initializeStruct(initializers: Partial<ArkMyStructComponent> | undefined): void {}
    build(builder: ((instance: ArkMyStructComponent) => void) | undefined) {}
}

class A<T extends A<T>>{
    baz: T;
    constructor(a0: T){
        this.baz = a0;
    }
    foo<S extends A<S>>(sVar: S, initializers: Partial<S>): void {
            sVar.bar(initializers)
            initializers.baz = /* @@ label */sVar
    }
    bar(initializers: Partial<T>): void {}
}

/* @@? 39:142 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 49:55 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 51:51 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 53:86 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 75:55 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 79:30 Error TypeError: T in Partial<T> must be a class or an interface type. */
