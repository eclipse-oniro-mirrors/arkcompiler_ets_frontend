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
class A {}
class B {}

let res: string = ""

class G <K, V> {
    get(key: K): K | V {
        res += "1"
        return key;
    }
}

class C extends G<A, B> {
    override get(k: A): B { // rossz
        res += "2"
        return new B();
    }
}

function main() {
    res = ""

    let x: G<A, B> = new C() as G<A, B>
    x.get(new A())

    arktest.assertEQ(res, "2")
}
