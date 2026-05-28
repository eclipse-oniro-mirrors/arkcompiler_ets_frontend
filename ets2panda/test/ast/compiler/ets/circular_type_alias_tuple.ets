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

type A = [A | undefined]
type B = [B]
type D = [[D]]
type F = [number, F]
type G = [G] | undefined
type H = [I]
type I = H
type J<T> = (a: J<T>) => T
type K = string | (() => K)
type L<T> = T
type M = L<M>

/* @@? 16:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 17:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 18:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 19:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 20:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 22:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 23:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 24:1 Error Semantic error ESE0311: Circular type alias reference */
/* @@? 26:1 Error Semantic error ESE0311: Circular type alias reference */