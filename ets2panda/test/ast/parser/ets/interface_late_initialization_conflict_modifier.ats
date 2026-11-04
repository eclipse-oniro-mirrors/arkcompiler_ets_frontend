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

interface A {
    f1?!:string
    f2!?:number
}
/* @@? 17:7 Error Syntax error ESY162966: Definite assignment assertions '!' are not allowed in interface properties. */
/* @@? 17:7 Error Syntax error ESY0127: Conflicting modifiers '!' and '?' on field. */
/* @@? 18:7 Error Syntax error ESY162966: Definite assignment assertions '!' are not allowed in interface properties. */
/* @@? 18:7 Error Syntax error ESY0127: Conflicting modifiers '!' and '?' on field. */
