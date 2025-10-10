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
files: ['./dummyModules/dummyModule1.ets', './dummyModules/dummyModule2.ets', './dummyModules/dummyModule3.ets', 
        './dummyModules/dummyModule4.ets', './dummyModules/dummyModule5.ets']
---*/

initModule('./dummyModules/dummyModule1')
initModule('./dummyModules/dummyModule2')
initModule('./dummyModules/dummyModule3')
initModule('./dummyModules/dummyModule4')
initModule('./dummyModules/dummyModule5')

function foo() {
    initModule('./dummyModules/dummyModule1')
    initModule('./dummyModules/dummyModule2')
    initModule('./dummyModules/dummyModule3')
    initModule('./dummyModules/dummyModule4')
    initModule('./dummyModules/dummyModule5')
}

function main() {
    for (let i = 0; i < 10; i++) {
        foo();
    }
}
