/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

class A<T> {
    memb: T = undefined as T;
    foo(a0: T): void {}
    bar(): T {
        return this.memb;
    }
}

function main(): void {
    let my_var: A<[int, String]> = new A<[int, String]>();
    my_var.memb = [1, "b"];
    let tup: [int, String] = [6, "c"];
    my_var.foo(tup);

    let my_var_2: A<[int, String]> = new A<[int, String]>();
    my_var_2.memb = my_var.bar();
}
