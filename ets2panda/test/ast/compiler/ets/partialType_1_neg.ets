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

/*
    Test case: T type from Partial<T> can not instantiate the Partial type.
    Explanation: Partial class is a completely different generated class, not connected to it's type parameter T.
                 Instantiating it with new T would require to have Partial<T> an effect on it's original
                 class T on modification (like any superclass relationship).
                 This behaviour isn't present here, as Partial<T> is not connected to T.
*/


class A {}

function foo(): void {
	let fld_1: Partial<A> = /* @@ label */new A(); // CTE
}


/* @@@ label Error Semantic error ESE0318: Type 'A' cannot be assigned to type 'Partial<A>' */