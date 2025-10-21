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

import {ClassAuthor, Color} from "./annotation_src"

@interface ClassAuthor {
    authorName: string = "Jim"
    /* @@ label */authorAge: /* @@ label1 */int = /* @@ label2 */32
    /* @@ label3 */testBool: /* @@ label4 */string = "false"
    /* @@ label5 */favorColor: Color
    /* @@ label6 */color: FixedArray<Color>
    reviewers: FixedArray<string> = ["Bob", "Jim", "Tom"]
    reviewersAge: int/* @@ label7 */[] = [18, 21, 32]
    testBools: FixedArray<boolean> = [false, true, false]
    mutiArray: FixedArray<FixedArray<number>> = [
        [1, 2, 3],
        [4, +5, 6],
        [7, 8, -9]
    ]
}

/* @@@ label1 Error TypeError: Field 'authorAge' has a type mismatch with the ambient annotation 'ClassAuthor'.  */
/* @@@ label2 Error TypeError: The initial value does not match the expected value.  */
/* @@@ label Error TypeError: Initializer for field 'authorAge' does not match the expected definition in the ambient annotation 'ClassAuthor'.  */
/* @@@ label4 Error TypeError: Field 'testBool' has a type mismatch with the ambient annotation 'ClassAuthor'.  */
/* @@@ label3 Error TypeError: Initializer for field 'testBool' does not match the expected definition in the ambient annotation 'ClassAuthor'.  */
/* @@@ label5 Error TypeError: Initializer for field 'favorColor' does not match the expected definition in the ambient annotation 'ClassAuthor'.  */
/* @@@ label6 Error TypeError: Initializer for field 'color' does not match the expected definition in the ambient annotation 'ClassAuthor'.  */
/* @@@ label7 Error TypeError: Field 'reviewersAge' has a type mismatch with the ambient annotation 'ClassAuthor'.  */