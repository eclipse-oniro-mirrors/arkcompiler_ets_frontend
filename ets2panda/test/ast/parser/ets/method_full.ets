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

abstract class method_full  {
    foo(): void {
    }
    foo_int/* @@ label */(b : double, c : char, i : int): int {
    }
    foo_bool/* @@ label1 */(d : double, c : char, ... i: int[]): boolean {
    }
    private final foo_private/* @@ label2 */(d : double): int {
    }
    public final foo_final/* @@ label3 */(d : double): double {
    }
    protected static foo_final/* @@ label4 */(i : int): boolean {
    }
    protected abstract foo_abstract(): int ;
    private native foo_native(): int ;
}


/* @@@ label Error Semantic error ESE0103: Function with a non void return type must return a value. */
/* @@@ label1 Error Semantic error ESE0103: Function with a non void return type must return a value. */
/* @@@ label2 Error Semantic error ESE0103: Function with a non void return type must return a value. */
/* @@@ label3 Error Semantic error ESE0103: Function with a non void return type must return a value. */
/* @@@ label4 Error Semantic error ESE0103: Function with a non void return type must return a value. */