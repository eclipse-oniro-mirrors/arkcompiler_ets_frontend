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

Class MyClass {
  compare((value: number, index:number, arr: Array<number>)) {}
}

/* @@? 16:1 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@? 16:7 Error Syntax error ESY0227: Unexpected token 'MyClass'. */
/* @@? 16:7 Error Semantic error ESE0143: Unresolved reference MyClass */
/* @@? 16:15 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 17:3 Error Semantic error ESE0143: Unresolved reference compare */
/* @@? 17:17 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 17:19 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 17:19 Error Syntax error ESY0227: Unexpected token 'number'. */
/* @@? 17:19 Error Semantic error ESE0144: Type name 'number' used in the wrong context */
/* @@? 17:25 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 17:27 Error Syntax error ESY0227: Unexpected token 'index'. */
/* @@? 17:33 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 17:33 Error Semantic error ESE0144: Type name 'number' used in the wrong context */
/* @@? 17:39 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 17:41 Error Syntax error ESY0227: Unexpected token 'arr'. */
/* @@? 17:46 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@? 17:46 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 17:59 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 17:60 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 17:62 Error Syntax error ESY0227: Unexpected token '{'. */
