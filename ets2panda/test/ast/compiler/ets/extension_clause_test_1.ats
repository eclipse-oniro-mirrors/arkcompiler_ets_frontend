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
class C extends MyPartial { }

type MyReadonly = Readonly<A>
class D extends MyReadonly { }

type MyRequired = Required<A>
class E extends MyRequired { }

type MyReturnType = ReturnType<Function>
class H extends MyReturnType { }

class B2 extends Partial<A> { }
class C2 extends Readonly<A> { }
class D2 extends Required<A> { }
class G1 extends ReturnType<Function> { }

/* @@? 22:17 Error Semantic error ESE0176: The super type of 'C' class is not extensible. */
/* @@? 25:17 Error Semantic error ESE0176: The super type of 'D' class is not extensible. */
/* @@? 28:17 Error Semantic error ESE0176: The super type of 'E' class is not extensible. */
/* @@? 31:17 Error Semantic error ESE0176: The super type of 'H' class is not extensible. */
/* @@? 33:18 Error Semantic error ESE0176: The super type of 'B2' class is not extensible. */
/* @@? 34:18 Error Semantic error ESE0176: The super type of 'C2' class is not extensible. */
/* @@? 35:18 Error Semantic error ESE0176: The super type of 'D2' class is not extensible. */
/* @@? 36:18 Error Semantic error ESE0176: The super type of 'G1' class is not extensible. */
