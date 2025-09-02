/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

interface itf {
    f1: ((s: string) => string)
    f2: ((() => number) | null)
    f3: (((() => void) | null) | String)
    f4: (((() => boolean) | null) | String)
}

class A implements itf {
    f1 = (s: string): string => {
        return s
    }
    f2 = null
    f3 = "f3"
    f4 = (): boolean => {
        return true;
    }
}


function main() {
    let a: A = new A()
    arktest.assertEQ(a.f1("f1"), "f1")
    arktest.assertEQ(a.f2, null)
    arktest.assertEQ(a.f3, "f3")
    arktest.assertEQ(a.f4(), true)
}
