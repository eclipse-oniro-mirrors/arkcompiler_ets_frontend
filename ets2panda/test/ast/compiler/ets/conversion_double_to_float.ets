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

function foo(a: float) : Int {
    return 0;
}

export const f1: float = 1.0;

function main() {
    const f2: float = 2.0;

    foo(3.0);
    foo(4.0);
    
    foo(1.0/0.0);
    foo(Infinity);
    foo(NaN);
}

/* @@? 28:5 Error TypeError: No matching call signature for foo(Double)  */
/* @@? 28:9 Error TypeError: Type 'Double' is not compatible with type 'Float' at index 1  */
/* @@? 29:5 Error TypeError: No matching call signature for foo(Double)  */
/* @@? 29:9 Error TypeError: Type 'Double' is not compatible with type 'Float' at index 1  */
/* @@? 30:5 Error TypeError: No matching call signature for foo(Double)  */
/* @@? 30:9 Error TypeError: Type 'Double' is not compatible with type 'Float' at index 1  */
