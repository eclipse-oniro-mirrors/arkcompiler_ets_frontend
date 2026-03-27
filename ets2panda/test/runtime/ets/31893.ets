/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

class C<T> {
    f(x: T, y: T, z: T): T[] {
        return [x, y, z].map((x) => x)
    }
}

function main() {
    let c = new C<int>
    let xyz = c.f(1, 2, 3)
    arktest.assertEQ(xyz[0], 1)
    arktest.assertEQ(xyz[1], 2)
    arktest.assertEQ(xyz[2], 3)
}
