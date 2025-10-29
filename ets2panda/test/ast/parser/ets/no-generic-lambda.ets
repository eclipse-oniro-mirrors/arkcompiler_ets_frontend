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

let foo = <T>(i: T) : void => { }
let bar = <T extends U, U>() => { }

foo<number>(1.0);
bar<Int, Object>();


/* @@? 16:11 Error Syntax error ESY12842: Generic lambda expressions are not supported. */
/* @@? 17:11 Error Syntax error ESY12842: Generic lambda expressions are not supported. */
/* @@? 19:1 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 20:1 Error Semantic error ESE0289: This expression is not callable. */