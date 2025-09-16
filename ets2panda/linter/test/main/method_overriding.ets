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

interface Person {
  cb: () => void
}

class student implements Person{
  cb() {}
}

interface I {
  cb: () => void
  cb2(): void;
}

class C implements I {
    cb() {} // Should report error
    cb2: () => void = () => {} // Should report error
}

class A {
    cb(): void {}
}

class B extends A {
    cb: () => void = () => {} // Should report error
}

interface Person7 {
  fun: () => void
}

interface Person7_1 extends Person7 {
  fun():void
}

interface Person8 {
  fun():void
}

interface Person8_1 extends Person8 {
  fun: () => void
}

interface Person9<T> {
  fun: () => void
}

interface Person9_1<T> extends Person9<T> {
  fun():void
}