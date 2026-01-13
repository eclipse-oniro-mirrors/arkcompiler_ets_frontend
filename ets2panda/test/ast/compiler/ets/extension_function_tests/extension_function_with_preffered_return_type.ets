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

class A {}
class B extends A {}

function receiver(this: A) {
    return this        
}

function fooB(b: B) {
}

function main() {
    let b = new B();
    let a = b.receiver()
    fooB(a)
}

/* @@? 29:5 Error Semantic error ESE0127: No matching call signature for fooB(A) */
/* @@? 29:10 Error Semantic error ESE0046: Type 'A' is not compatible with type 'B' at index 1 */
