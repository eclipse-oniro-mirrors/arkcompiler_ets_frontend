/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

let x: int[ = [1,2,3]
let y: int = x [0]


/* @@? 16:13 Error Syntax error ESY0228: Unexpected token, expected ']'. */
/* @@? 16:13 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 16:13 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 16:17 Error Syntax error ESY0228: Unexpected token, expected ']'. */
/* @@? 16:18 Error Syntax error ESY0227: Unexpected token '2'. */
/* @@? 16:19 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 16:20 Error Syntax error ESY0227: Unexpected token '3'. */
/* @@? 16:21 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 17:14 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */