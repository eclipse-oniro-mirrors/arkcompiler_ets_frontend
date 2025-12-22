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

class A {
    get abc(x: number) {}
    set abc(x: number, a: string) {}
}

function fun(this: A) {}

class int {}

/* @@? 17:12 Error Syntax error ESY0058: Getter must not have formal parameters. */
/* @@? 17:12 Error Semantic error ESE0086: Getter must return a value */
/* @@? 18:12 Error Syntax error ESY0063: Setter must have exactly one formal parameter. */
/* @@? 23:7 Error Syntax error ESY0295: int is a predefined type, cannot be used as an identifier */
