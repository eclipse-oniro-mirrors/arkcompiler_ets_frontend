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

interface Ifoo {
    foo: int;
    callFunc(): int {

        this.foo = 1;
        let bar:int =1;
        let callback = () => {
            bar = 2;
            return this.foo + bar;
        }
        return callback();
    }
}

class A implements Ifoo {
    foo: int;
}

function main() {
    let foo = new A();
    foo.callFunc();
    arktest.assertEQ(foo.callFunc(), 3);
}
