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
    foo(p: I): void
    foo(p: C): void
}

/* @@? 22:5 Error Semantic error ESE602430: Method foo(p: C): undefined from class or interface C, inherited into class or interface C, conflicts with the previous declaration of method foo(p: I): undefined from class or interface C because both declarations override method foo(p: C): undefined from class or interface I */
/* @@? 22:5 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(p: C) => undefined' will never be selected for a call, as the preceding entity with the signature '(p: I) => undefined' prevails. */
