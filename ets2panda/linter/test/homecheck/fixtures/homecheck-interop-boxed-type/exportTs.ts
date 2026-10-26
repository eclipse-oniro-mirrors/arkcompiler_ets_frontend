/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export class ClassBoxedInField {
    booleanBox: Boolean = new Boolean(false);

    foo() {}
}

export class ClassBoxedInMethod {
    fieldA = false;

    foo(): String {
        return new String('123');
    }
}

export class ClassNoBoxed {
    fieldA = '123';
    fieldB = String('abc');

    foo(): String {
        return '123';
    }
}

export namespace myNamespaceA {
    export const a = new String('abc');
}

export namespace myNamespaceB {
    export function foo() {
        return new Number(123);
    }
}

export namespace myNamespaceC {
    export class ClassBoxedInMethod {
        fieldA = false;
        foo(): String {
            return new String('123');
        }
    }
}

export namespace myNamespaceD {
    export namespace nestedNS {
        export const a: String = 'abc';
        export function foo(): Number {
            return 123;
        }
        export class ClassBoxedInField {
            booleanBox: Boolean = false;
        }
        export class ClassBoxedInMethod {
            foo(): String {
                return '123';
            }
        }
    }
}

export class Node {
    parents: Node[];
    children: Node[];
    value = new String('abc');
}
