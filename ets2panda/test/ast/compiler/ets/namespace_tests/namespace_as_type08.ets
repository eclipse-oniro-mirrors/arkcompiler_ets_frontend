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
namespace MySpace {
    let a:string = "1"
}

let foo1:(a:MySpace)=>int
let foo2:()=>MySpace

/* @@? 19:13 Error Semantic error ESE0158: Namespace 'MySpace' cannot be used as a type. */
/* @@? 20:14 Error Semantic error ESE0158: Namespace 'MySpace' cannot be used as a type. */