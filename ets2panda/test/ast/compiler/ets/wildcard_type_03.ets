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

interface  X<out T extends C> {
    read(): T;
}

function foo(obj: object) {
    if (obj instanceof X) {
        let o1: X<C> = obj;				// OK
        let o2: X<never> = /* @@ label */obj;	// CTE

        let p: C = obj.read();			// OK
    }
}


/* @@@ label Error Semantic error ESE0318: Type 'X<C>' cannot be assigned to type 'X<never>' */
