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

interface I {
    foo(p: C): void
}

interface C extends I {
    foo(p: C): void
    foo(p: I): void
}

/* @@? 22:5 Error Semantic error ESE602430: Method foo(p: I): void from class or interface C, inherited into class or interface C, conflicts with the previous declaration of method foo(p: C): void from class or interface C because both declarations override method foo(p: C): void from class or interface I */
