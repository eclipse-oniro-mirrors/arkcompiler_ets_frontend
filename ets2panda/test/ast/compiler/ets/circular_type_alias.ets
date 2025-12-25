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

type X<T extends R, R extends T, U extends T> =U<T<R>>

type Y<T extends T> = Y<T>

/* @@? 16:31 Error Semantic error ESE0184: Type parameter 'R' has circular constraint dependency. */
/* @@? 18:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 18:18 Error Semantic error ESE0184: Type parameter 'T' has circular constraint dependency. */