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

namespace NS {
    get name(): string {return 'hello'; }
    set name(arg: string) { 
        if (arg == '111') {
            arktest.assertTrue(true);
        } else {
            throw Error('Not correct name');
        }
    }
    export function foo() {
        arktest.assertEQ(name, 'hello');
        name = '111'
        arktest.expectError((): void => {name = '222'}, Error('Not correct name'))
    }
}

function main() {
    NS.foo();
}
