/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

abstract class A {
    abstract foo(): void;
}

class H {

}

class B {
    // It is forbidden to extend a class by an anonymous class
    bar: A = new A() {
        override foo(): void {

        }
    }
    // It is forbidden to extend a class by an anonymous class
    func: H = new H() {

    }
}
