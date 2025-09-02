/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
    foo(): int {
        return 2;
    }
}

function main(): void {
    let a = [new Object(), new Object(), new Object()]
    a[0] = "b"
    a[1] = new A()
    arktest.assertEQ(a[0], "b")
    arktest.assertEQ((a[1] as A).foo(), 2)
    let b = [new Object()]
    b[0] = 5.0
    arktest.assertEQ(b[0] as Number + 5, 10)
    let c = ["a", "b"]
    arktest.assertEQ(c[0], "a")
    arktest.assertEQ(c[1], "b")
    c[0] = "b"
    c[1] = "c"
    arktest.assertEQ(c[0], "b")
    arktest.assertEQ(c[1], "c")
    let d = [1, 2, 3]
    d[0] = 2.5
    arktest.assertEQ(d[0], 2.5)
    arktest.assertEQ(d[0] + d[1], 4.5)
}
