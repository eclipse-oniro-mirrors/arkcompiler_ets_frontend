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

// (arkts-no-enum-mixed-types)
 enum E {
    A = 0xa,
    B = 0xb,
    C,
    D = Math.random(),
    E = "0xd",
}

/* @@? 21:9 Error Semantic error ESE0342: Enumeration members can be initialized only by compile-time expressions and initializers must be of the same type. */
/* @@? 22:9 Error Semantic error ESE0342: Enumeration members can be initialized only by compile-time expressions and initializers must be of the same type. */