/**
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

import {bytecodehar2test} from "@normalized:N&&&bytecodehar2/bytecodehar2-file1&1.0.0"
import {test3} from '@normalized:N&&&bytecodehar1/bytecodehar1-file3&1.0.0'
import {addNewFile} from "@normalized:N&&&bytecodehar1/bytecodehar1-file4&1.0.0"


export function test2() {
    print('bytecodehar1.file2.test');
    bytecodehar2test();
    test3();
    addFunction();
    import("@normalized:N&&&sourcehar/sourcehar-file2&1.0.0")
}

function addFunction() {
    print('addFunction');
    import("@normalized:N&&&sourcehar/sourcehar-file2&1.0.0");
    test1();
    addNewFile();
}