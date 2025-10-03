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

class A<T> {
    f1!: T // cte
}

class B extends A<string>{

}

class C<T extends Object> extends A<T>{
    f1!: T //OK
}

class D<T extends string> extends A<T>{
    f1!: T //OK
}

class E<T> extends A<T>{
    f1!: T // cte
}


/* @@? 17:10 Error Semantic error ESE0376: Late-initialized field cannot be nullish types or possibly nullish types. */
/* @@? 33:10 Error Semantic error ESE0376: Late-initialized field cannot be nullish types or possibly nullish types. */