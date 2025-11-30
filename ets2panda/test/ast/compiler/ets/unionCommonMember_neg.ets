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

class A {
    fld1: Int = 42
    fld2: double = 42.0
    fld3: [string, number] = ["abc", 42.0]
    fld4: [string, string] = ["def", "42.0"]
    fld5: string | number = 42.0
    fld6: string | number = 42.0
    fld7: string | number | null = 42.0
    fld8: Map<string, string> = new Map<string, string>()
}

class B {
    fld1: Number = 33.0
    fld2: Double = 33.0
    fld3: string[] = ["ghi", "jkl"]
    fld4: string[] = ["mno", "pqr"]
    fld5: string | number | null = 33.0
    fld6: string | number | undefined = 33.0
    fld7: string | number | undefined = 33.0
    fld8: Map<string, number> = new Map<string, number>()
}

function getUnion(): A | B {
    return new A
}

function main() {
    const u = getUnion()
    arktest.assertEQ(/* @@ label1 */u.fld1, 42)
    arktest.assertEQ(/* @@ label2 */u.fld2, 42.0)
    arktest.assertEQ(/* @@ label3 */u.fld3[0], "abc")
    arktest.assertEQ(/* @@ label4 */u.fld4[0], "def")
    arktest.assertEQ(/* @@ label5 */u.fld5, 42.0)
    arktest.assertEQ(/* @@ label6 */u.fld6, 42.0)
    arktest.assertEQ(/* @@ label7 */u.fld7, 42.0)
    arktest.assertEQ(/* @@ label8 */u.fld8, new Map<string, string>())
}


/* @@@ label1 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@@ label3 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@@ label4 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@@ label5 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@@ label6 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@@ label7 Error Semantic error ESE0244: Member type must be the same for all union objects. */
/* @@@ label8 Error Semantic error ESE0244: Member type must be the same for all union objects. */
