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

import {test1} from '@normalized:N&&&bytecodehar2/bytecodehar2-file1&1.0.0'
import {test2} from '@normalized:N&&&bytecodehar1/bytecodehar1-file2&1.0.0'
import {test3} from '@normalized:N&&&bytecodehar1/bytecodehar1-file3&1.0.0'

export function run() {
    print('bytecodehar1.file1.run');
    test2();
    test3();
    import('@normalized:N&&&sourcehar/sourcehar-file1&1.0.0');
}
