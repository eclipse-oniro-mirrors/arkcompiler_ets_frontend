/*
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

interface IX {
    x: int
}

interface IY {
    y: int
}

class XYImpl implements IX, IY {
    x: int = 1
    y: int = 2
}

function main(): void {
    let c = new XYImpl();
    let ix: IX = c;
    let iy: IY = c;

    arktest.assertEQ(ix.x, 1);
    arktest.assertEQ(iy.y, 2);

    ix.x = 10;
    iy.y = 20;

    arktest.assertEQ(ix.x, 10);
    arktest.assertEQ(iy.y, 20);
    arktest.assertEQ(c.x, 10);
    arktest.assertEQ(c.y, 20);
}
