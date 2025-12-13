/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class C {
    constructor (a: int) {}
}

let c: C = /* @@ label */{};


/* @@@ label Error Semantic error ESE0291: Type C has no parameterless constructor. Initialization with literals is not supported if the type has no parameterless constructor. Declare the parameterless constructor explicitly or remove parametered constructors! */