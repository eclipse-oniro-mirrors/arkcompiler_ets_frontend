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

type A<T> = T[] //ok

type B<T> = A<A<T>> //ok

type X<U, R extends U, T extends U = R> = /* @@ label1 */T</* @@ label2 */R<U>>


/* @@@ label1 Error Semantic error ESE0169: Type 'T' is not generic. */
/* @@@ label2 Error Semantic error ESE0169: Type 'R' is not generic. */

