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

type A<T, C, U> = T | T[] | A<C, U, T>[];

function main(): int {
    let bad = [1 as Int] as A<Int, Boolean, string>[];
    let value: A<string, Int, Boolean>[] = bad;
    return value.length;
}

/* @@? 20:44 Error Semantic error ESE0318: Type 'Array<Int|Array<Int>|Array<A<Boolean,String,Int>>>' cannot be assigned to type 'Array<String|Array<String>|Array<A<Int,Boolean,String>>>' */
