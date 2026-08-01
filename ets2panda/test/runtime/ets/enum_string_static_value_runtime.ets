/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

const v1: string = "11"
const v2: "12" = "12"
const v3: "13" = "13"

enum InferredStringEnum {
    FIRST = v1,
    SECOND = v2,
    THIRD = v3,
}

enum ExplicitStringEnum: string {
    FIRST = v1,
    SECOND = v2,
    THIRD = v3,
}

function main(): void {
    arktest.assertTrue(InferredStringEnum.FIRST instanceof InferredStringEnum)
    arktest.assertTrue(InferredStringEnum.SECOND instanceof InferredStringEnum)
    arktest.assertTrue(InferredStringEnum.THIRD instanceof InferredStringEnum)
    arktest.assertTrue(InferredStringEnum.FIRST.valueOf() instanceof string)
    arktest.assertTrue(InferredStringEnum.SECOND.valueOf() instanceof string)
    arktest.assertTrue(InferredStringEnum.THIRD.valueOf() instanceof string)
    arktest.assertEQ(InferredStringEnum.FIRST.valueOf(), "11")
    arktest.assertEQ(InferredStringEnum.SECOND.valueOf(), "12")
    arktest.assertEQ(InferredStringEnum.THIRD.valueOf(), "13")

    arktest.assertTrue(ExplicitStringEnum.FIRST instanceof ExplicitStringEnum)
    arktest.assertTrue(ExplicitStringEnum.SECOND instanceof ExplicitStringEnum)
    arktest.assertTrue(ExplicitStringEnum.THIRD instanceof ExplicitStringEnum)
    arktest.assertTrue(ExplicitStringEnum.FIRST.valueOf() instanceof string)
    arktest.assertTrue(ExplicitStringEnum.SECOND.valueOf() instanceof string)
    arktest.assertTrue(ExplicitStringEnum.THIRD.valueOf() instanceof string)
    arktest.assertEQ(ExplicitStringEnum.FIRST.valueOf(), "11")
    arktest.assertEQ(ExplicitStringEnum.SECOND.valueOf(), "12")
    arktest.assertEQ(ExplicitStringEnum.THIRD.valueOf(), "13")
}
