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

export interface I {
    i: number
}

export class A implements I {
    i: number = 0;
    a: string = 'a';
}

export class B implements I {
    i: number = 0;
    b: string = 'b';
}

let a: A = new A();
let i: I = a;
export { i };

export class C {
    static c: I = a;
}

export interface ImportedBaseI {}

export class ImportedParent implements ImportedBaseI {
    name: string = 'parent';
}

export class ImportedParent1 implements ImportedBaseI {
    name: string = 'parent1';
}

export let importedParent: ImportedParent = new ImportedParent();
export let importedBase: ImportedBaseI = importedParent;