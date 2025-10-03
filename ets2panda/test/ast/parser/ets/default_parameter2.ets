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


/* @@@ label Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@@ label Error Semantic error ESE0128: No matching call signature */