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

class C {}

interface  X<in T extends C> {
    write(P: T): void;
}

function foo(obj: object) {
    if (obj instanceof X) {
        let o1: X<C> = /* @@ label1 */obj;				// CTE
        let o2: X<never> = obj;					// OK

        /* @@ label2 */obj.write(/* @@ label3 */new C());		// CTE
    }
}


/* @@@ label1 Error Semantic error ESE0318: Type 'X<never>' cannot be assigned to type 'X<C>' */
/* @@@ label2 Error Semantic error ESE0127: No matching call signature for write(C) */
/* @@@ label3 Error Semantic error ESE0046: Type 'C' is not compatible with type 'never' at index 1 */
