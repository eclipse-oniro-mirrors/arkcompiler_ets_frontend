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

import { MyClass } from './import_interface_test_2';
import { SameInterface } from './import_interface_test_1';

class MyClass2 implements SameInterface {
    test(): int {
        return 2;
    }
}

function main() {
    const myclass1 = new MyClass();
    const myclass2 = new MyClass2();
    if ((myclass1.test() + myclass2.test()) == 3) {
        return 0
    }
    return 1
}
