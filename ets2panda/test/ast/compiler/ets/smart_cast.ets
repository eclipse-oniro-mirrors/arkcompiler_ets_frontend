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

function foo(p: "Object" | "String" | "Number" = "Object") { // should be ok
}

function bar(p: "Object") {
    let s: "Object" = p != undefined ? p : "Object"// should be ok
}

function baz(p: "Double") {
    let s: "Object" = p != undefined ? p : "Double"// should be cte
}



/* @@? 24:23 Error Semantic error ESE0318: Type '"Double"' cannot be assigned to type '"Object"' */