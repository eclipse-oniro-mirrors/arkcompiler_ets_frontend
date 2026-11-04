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

class AA {
    build() {
        const classA = new A();
        classA.a<(sr: string) => void>((sr: string) => {
            if (sr != "Hello") {
                arktest.assertTrue(false);
            }
        })
    }
}

class A {
    a<T extends Function>(a3: T) : void {
        a3.unsafeCall("Hello");
    }
}

function main() {
    const aa = new AA();
    aa.build();
}