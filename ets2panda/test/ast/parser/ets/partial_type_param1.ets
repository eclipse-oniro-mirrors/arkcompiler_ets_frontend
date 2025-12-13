/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface I {
    i: number;
    j: string;
}

class CI implements I {
    _i: number = 0;
    _j: string = "";

    set i(i: number) { this._i = i; }
    get i(): number { return this._i; }

    set j(j: string) { this._j = j; }
    get j(): string { return this._j; }
}

function generic<T extends I>(t: Partial<T>): boolean {
    return t !== null && t !== undefined;
}

function main() {
    // Test that {i: 1} is accepted as Partial<CI>
    generic<CI>({i: 1});
}

/* @@? 32:41 Error Semantic error ESE124644: T in Partial<T> must be a class or an interface type. */
