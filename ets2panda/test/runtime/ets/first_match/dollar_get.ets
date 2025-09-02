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
    $_get1(x: number): int {
        return x.toInt();
    }

    $_get2(x: string): string {
        return x;
    }

    overload $_get { $_get1, $_get2 }
}

function main() {
    let a = new A();

    arktest.assertEQ(a[1], 1)
    arktest.assertEQ(a["abc"], "abc")
}
