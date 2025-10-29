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

interface A<T> {}

class Obb {
    public static foos<T = undefined>(a : A<T>) : A<T> {
        return a;
    }
}
Obb.foos(){}


Error().fromEntries() {}

/* @@? 23:1 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 23:1 Error Semantic error ESE0128: No matching call signature */
/* @@? 26:1 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 26:1 Error Semantic error ESE0128: No matching call signature */
/* @@? 26:9 Error Semantic error ESE0207: 'fromEntries' is a static property of 'Error' */