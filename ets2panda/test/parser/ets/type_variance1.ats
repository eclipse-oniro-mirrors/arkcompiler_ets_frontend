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

class C<in T> {
    set(_: T): void {}
}

const C_GLOBAL: C<Object[]> = new C<Object[]>();

interface Declaration_Site_I_O<in In_T, out Out_T> {
    set(_: In_T): void;
    get(): Out_T;
}

class O {}

class Declaration_Site_C implements Declaration_Site_I_O<O, O> {
    override set(_: O): void {}
    override get(): O {
        return new O();
    }
}

class O2<T> {}

function test(): void {
    const o: O2<Object> = new O2<Object>();
    const o2: O2<Object[]> = new O2<Object[]>();
}
