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
 
 function foo(a: Int|undefined, b: Int|undefined): Int {
    return /* @@ label */a * b;
}

function main() {
    let result = foo(undefined, undefined);
}


/* @@@ label Error Semantic error ESE0106: Bad operand type: multiple types left in the normalized union type (Int|undefined). Unions are not allowed in binary expressions except equality. */