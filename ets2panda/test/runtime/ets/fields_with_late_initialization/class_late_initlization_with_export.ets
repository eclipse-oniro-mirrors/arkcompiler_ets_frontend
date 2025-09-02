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

/*---
  files: ['./interfaceToExport/interfaceToExport.ets']
---*/

import { A } from './interfaceToExport/interfaceToExport.ets';

class Aimpl implements A {
    a: number = 0;
    b!: string;
}

function main() {
    try {
        let a: Aimpl = new Aimpl();
        console.log('a.b = ' + a.b);
    } catch (e) {
        arktest.assertTrue(e instanceof NullPointerError);
    }
}
