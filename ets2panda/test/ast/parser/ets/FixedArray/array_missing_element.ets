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

function foo(): void {
  let b: FixedArray<number|undefined> = [ 1, 2, 3,  , 4, ]
}


/* @@? 17:53 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 17:55 Error Syntax error ESY0229: Unexpected token, expected ',' or ']'. */
/* @@? 17:55 Error Syntax error ESY0227: Unexpected token '4'. */
/* @@? 17:56 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 17:58 Error Syntax error ESY0227: Unexpected token ']'. */