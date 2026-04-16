/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

let res = 0;

class A {
    foo<T extends A>(x: T): void {
        res = 1
    }
}

class B extends A {
    override foo<T extends B>(x: T) {
       res = 2
    }
}

function main() {
    new B().foo<B>(new B())
    arktest.assertEQ(res, 2);
}


/* @@? 25:17 Error Semantic error ESE150729: Class member foo with type undefined in B cannot override foo with type undefined field from class A because the overriding type parameter's constraints are not compatible with type parameter constraints of the overridden method. */
/* @@? 25:17 Error Semantic error ESE0136: Method foo<T>(x: T): undefined in B not overriding any method */
