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
    b: number = 111;
    constructor(b: number) {
        this.b = b;
    }
    test() : string {
        return "func"
    }
}
try {
    let a = new A(1);
    a.b();
} catch (e) {
    print(e.message);
    print(e.stack);
}
try {
    let a = new A();
    a.test()();
} catch (e) {
    print(e.message);
    print(e.stack);
}
