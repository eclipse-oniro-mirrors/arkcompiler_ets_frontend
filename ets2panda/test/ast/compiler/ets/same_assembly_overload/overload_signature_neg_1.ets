/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

//declare function has body
export class A<T> {}
export declare function foo(a:A<number>):void
export declare function foo(a:A<string>):number {}


/* @@? 18:1 Warning Warning W0022: Function foo with this assembly signature already declared. */
/* @@? 19:49 Error Syntax error ESY0331: Ambient functions, constructors, and methods cannot have bodies. Ambient modules and namespaces can contain only ambient declarations. */
/* @@? 19:49 Error Semantic error ESE0083: Native, Abstract and Declare methods cannot have body. */
