/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

function main() : void
{
    /* @@ label */foo();
}

function foo(a : int, b : int = 10) : int
{
    return a + b;
}

function foo(a : int) : int
{
    return a;
}

/* @@? 18:19 Error Semantic error ESE0127: No matching call signature for foo() */
/* @@? 26:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Int) => Int' will never be selected for a call, as the preceding entity with the signature '(a: Int, _: Int|undefined) => Int' prevails. */
