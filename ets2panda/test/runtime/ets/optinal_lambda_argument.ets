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

let count = 1;
class A {
    callback_?: Function;
    on(callback?: Function): void {
        this.callback_ = callback;
        this.callback_!.unsafeCall();
    }
}

class B {
    callback1_?: Function;
    callback2_?: Function;
    on(callback1?: Function, callback2?: Function): void {
        this.callback1_ = callback1;
        this.callback1_!.unsafeCall();
        this.callback2_ = callback2;
        this.callback2_!.unsafeCall();
    }
}

function main() {
    let a = new A();
    a.on(() => { count = 100 })
    arktest.assertEQ(count, 100)

    let lambda1 = () => { count = 200 };
    a.on(lambda1);
    arktest.assertEQ(count, 200)

    let b = new B();
    b.on(() => { count = 300 }, () => { count = 400 });
    arktest.assertEQ(count, 400)
}
