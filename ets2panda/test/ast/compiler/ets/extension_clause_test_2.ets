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
    a: int = 1
    b: string = 'a'
}

type MyPartial = Partial<A>
type PartialCopy = MyPartial
class C extends PartialCopy { }

type MyReadonly = Readonly<A>
type ReadonlyCopy = MyReadonly
class D extends ReadonlyCopy { }

type MyRequired = Required<A>
type RequiredCopy = MyRequired
class E extends RequiredCopy { }

type MyReturnType = ReturnType<Function>
type ReturnTypeCopy = MyReturnType
class H extends ReturnTypeCopy { }

/* @@? 23:17 Error Semantic error ESE0176: The super type of 'C' class is not extensible. */
/* @@? 27:17 Error Semantic error ESE0176: The super type of 'D' class is not extensible. */
/* @@? 31:17 Error Semantic error ESE0176: The super type of 'E' class is not extensible. */
/* @@? 35:17 Error Semantic error ESE0176: The super type of 'H' class is not extensible. */
