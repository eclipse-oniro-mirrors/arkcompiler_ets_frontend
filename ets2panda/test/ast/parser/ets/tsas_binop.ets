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

enum test{
    a,
}

function main():void{
    (1.0 as test) == test.a
}


/* @@? 21:6 Error Semantic error ESE0046: Type 'Double' is not compatible with type 'Int' at index 1 */
/* @@? 21:6 Error Semantic error ESE0127: No matching call signature for fromValue(Double) */
/* @@? 21:6 Warning Warning W0030: Enum cast is deprecated. Cast support from 'Double' to 'test' will be removed. */