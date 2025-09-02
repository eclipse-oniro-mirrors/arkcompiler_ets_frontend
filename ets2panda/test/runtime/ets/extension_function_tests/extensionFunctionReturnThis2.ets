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

class B {
    x: int
    setX(x:int): this { this.x = x; return this }
}
class D extends B {
    y: int
    setY(y:int): this { this.y = y; return this }
}
function rotate(this: B): this { return this }

new D().setX(3).rotate().setY(33);
arktest.assertTrue(true)
let d = new D();
d.setX(3).rotate().setY(33);
arktest.assertEQ(d.x, 3)
arktest.assertEQ(d.y, 33)
