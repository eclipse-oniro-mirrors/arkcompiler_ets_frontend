/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
class B {}

class A1<in T> {}
class A2<out T> {}

function main(): void {
    let x4: A2<A> = /* @@ label */new A2<B>()
}

/* @@@ label Error Semantic error ESE0318: Type 'A2<B>' cannot be assigned to type 'A2<A>' */