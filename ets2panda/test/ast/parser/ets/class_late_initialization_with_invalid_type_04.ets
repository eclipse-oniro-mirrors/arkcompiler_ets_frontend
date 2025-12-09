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

interface A<T> {
    f1!: T
}

class B implements A<string>{
    f1!: string
}

class C<T extends Object> implements A<T>{
    f1!: T //OK
}

class D<T extends string> implements A<T>{
    f1!: T //OK
}

class E<T> implements A<T>{
    f1!: T // cte
}

/* @@? 17:7 Error Syntax error ESY162966: Definite assignment assertions '!' are not allowed in interface properties. */
/* @@? 33:10 Error Semantic error ESE0376: Late-initialized field cannot be nullish types or possibly nullish types. */
