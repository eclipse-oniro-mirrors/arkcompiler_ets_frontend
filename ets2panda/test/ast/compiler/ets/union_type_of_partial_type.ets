/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

abstract class A<T extends A<T>>{
    foo({ }, init: Partial<T> | undefined): void {
    }
}


/* @@? 16:9 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 16:9 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 16:9 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 16:12 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 16:27 Error Semantic error ESE124644: T in Partial<T> must be a class or an interface type. */
/* @@? 16:42 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 16:43 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 16:45 Error Syntax error ESY0295: void is a predefined type, cannot be used as an identifier */
/* @@? 16:50 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 18:1 Error Syntax error ESY0227: Unexpected token '}'. */
