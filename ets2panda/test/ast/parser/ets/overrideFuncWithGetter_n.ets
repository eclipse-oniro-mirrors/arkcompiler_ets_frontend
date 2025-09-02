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

/* @@ label */interface A {
    length(): number;
}

class C implements A /* @@ label1 */{
    override get length/* @@ label2 */(): number {
        return 0;
    }
}

/* @@@ label Error TypeError: Cannot inherit from interface A because method length is inherited with a different declaration type */
/* @@@ label1 Error TypeError: C is not abstract and does not override abstract method length(): Double in A */
/* @@@ label2 Error TypeError: Method length(): Double in C not overriding any method */
