/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

interface C {
    AAA : number
}

class A  {
    AAA : number = 2
}

class B extends A implements C{
    AAA = 2
}

/* @@? 25:5 Error Semantic error ESE985118: Class member AAA with type Int in B cannot implement AAA with type Double property from interface C because the implementing return type is not compatible with the interface return type. */
/* @@? 25:5 Error Semantic error ESE0136: Method AAA(): Int in B not overriding any method */
/* @@? 25:5 Error Semantic error ESE0136: Method AAA(AAA: Int): undefined in B not overriding any method */
