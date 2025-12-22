/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class B<BT>{}
class A<T> {
    static a0: B<T>;
    static a1: T[];
    static a2: [T];
    static a3: (a1: T) => T;
    static a4: T;
}

/* @@? 18:12 Error Semantic error ESE0280: Property 'a0' might not have been initialized. */
/* @@? 19:12 Error Semantic error ESE0280: Property 'a1' might not have been initialized. */
/* @@? 20:12 Error Semantic error ESE0280: Property 'a2' might not have been initialized. */
/* @@? 21:12 Error Semantic error ESE0280: Property 'a3' might not have been initialized. */
