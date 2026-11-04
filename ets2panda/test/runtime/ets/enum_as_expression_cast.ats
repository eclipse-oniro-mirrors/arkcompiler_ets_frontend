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

enum E {
    A = 100, 
    B = 200
}

function main() {
    arktest.assertEQ(E.A.toString(), "100")             // E.A.toString()                      
    arktest.assertEQ(E.A, 100)                          // cast(E.A) to int, E.A is a index    
    arktest.assertEQ(E.B.toString(), "200")             // E.B.toString()                      
    arktest.assertEQ(Int.toDouble(E.B), 200)            // cast(E.B) to number, E.B is a index 
    arktest.assertEQ(E.A.valueOf(), 100)                // get value of E.A                    
    arktest.assertEQ(Int.toDouble(E.B.valueOf()), 200)  // get value of E.B                    
}