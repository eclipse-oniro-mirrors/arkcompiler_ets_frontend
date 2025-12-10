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

class A<CTE> {
    static bypass(input:CTE): CTE {
        return input;
    }
}

arktest.assertEQ(A.bypass(1), 1)

/* @@? 22:18 Error Semantic error ESE0127: No matching call signature for bypass(Int) */
/* @@? 22:27 Error Semantic error ESE0046: Type 'Int' is not compatible with type 'CTE' at index 1 */
