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

enum Color {Red = "10"}


function main() {
    arktest.assertEQ(~/* @@ label */Color.Red, ~10)
    arktest.assertEQ(+/* @@ label1 */Color.Red, 10)
    arktest.assertEQ(-/* @@ label2 */Color.Red, -10)
    // This was allowed because enum is in extended conditional expression which is expected.
    arktest.assertEQ(!Color.Red, !10)
}

/* @@@ label Error TypeError: Bad operand type, the type of the operand must be numeric type.  */
/* @@@ label1 Error TypeError: Bad operand type, the type of the operand must be numeric type.  */
/* @@@ label2 Error TypeError: Bad operand type, the type of the operand must be numeric type.  */
