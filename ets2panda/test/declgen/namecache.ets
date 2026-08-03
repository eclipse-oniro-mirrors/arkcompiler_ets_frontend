/**
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

export function foo(): void {
}

export function foo(num: int): void {
}

function foo1(): void {
}

function foo1(num: int): void {
}

// Prefix-collision regressions for nameCache signature rewriting:
// std.core.Int must not rewrite longer names (Intl.*, InternalError, Int8Array).
export function useIntl(fmt: Intl.DateTimeFormat): void {
}

export function useInternalError(err: InternalError): void {
}

export function useInt8Array(arr: Int8Array): void {
}

export function useIntArray(arr: int[]): void {
}

export interface IBase {
    baseField: string;
    baseMethod(): void;
}

export interface IShape extends IBase {
    id: int;
    name?: string;
    area(): number;
    resize(w: int, h: int): void;
    get tag(): string;
    set tag(v: string);
}

export function useInterface(shape: IShape): void {
}

export namespace Utils {
    export const NS_CONST: int = 1;

    export function nsFoo(x: int): string {
        return '';
    }

    export interface INsItem {
        value: int;
        describe(): string;
    }

    export class NsBox {
        public data: int = 0;

        public getData(): int {
            return this.data;
        }

        public setData(v: int): void {
            this.data = v;
        }
    }

    export namespace Inner {
        export class InnerCls {
            public ping(): void {
            }
        }

        export function innerFoo(): void {
        }
    }
}

export function useNsType(item: Utils.INsItem): void {
}

export @interface Anno {
    a: int;
    b: string;
}

@Anno({ a: 0, b: 'str' })
export class ClassA {
    public static sField: int = 0;
    public field1: string = 'a';
    field2: number = 0;
    
    constructor() {
        this.field2 = 2;
    }
    
    public method1(): void {
    }
    
    public method1(str: string): void {
    }
    
    public method2(): void {
    }
    
    public method2(num: int): void {
    }

    set age(v: number) {
    }

    get age(): number {
        return 1;
    }
}

export enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 3
}

export const VAR1: number = 0;
export const VAR2: string = 'ArkTS';
