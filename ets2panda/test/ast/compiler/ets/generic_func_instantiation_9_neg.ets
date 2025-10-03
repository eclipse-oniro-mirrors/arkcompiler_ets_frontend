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

function foo<T extends String, U = Int>(a0: T, a1: U): U { return a1; }

function main() {
    let foo1 = foo<Double>
    let foo2 = foo<Int, String>
}

/* @@? 19:19 Error Semantic error ESE0228: Type argument 'Double' should be a subtype of 'String' constraint */
/* @@? 20:19 Error Semantic error ESE0228: Type argument 'Int' should be a subtype of 'String' constraint */
