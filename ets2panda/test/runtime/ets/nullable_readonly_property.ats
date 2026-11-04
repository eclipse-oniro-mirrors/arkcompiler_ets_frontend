/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

export interface Actions {
    readonly type: string;
    readonly id: string;
}

export interface VerifyCodeFixAvailableOptions {
    readonly description: string;
    readonly actions?: ReadonlyArray<Actions>;
    readonly commands?: ReadonlyArray<string>;
}

class verify {
    static codeFixAvailable(options?: ReadonlyArray<VerifyCodeFixAvailableOptions>): void {
        arktest.assertEQ(options![0].description, "Implement interface 'I1'");
        arktest.assertEQ(options![0].actions, undefined);
        arktest.assertEQ(options![0].commands, undefined);
        arktest.assertEQ(options![1].description, "Implement interface 'I2'");
        arktest.assertEQ(options![1].actions, undefined);
        arktest.assertEQ(options![1].commands, undefined);
        arktest.assertEQ(options![2].description, "Implement interface 'I3'");
        arktest.assertEQ(options![2].actions![0].type, "IA");
        arktest.assertEQ(options![2].actions![0].id, "IA1");
        arktest.assertEQ(options![2].commands, undefined);
        arktest.assertEQ(options![3].description, "Implement interface 'I4'");
        arktest.assertEQ(options![3].actions![0].type, "IB");
        arktest.assertEQ(options![3].actions![0].id, "IB1");
        arktest.assertEQ(options![3].commands![0], "CMD1");
    };
}

function main() {
    verify.codeFixAvailable([
        { description: "Implement interface 'I1'" },
        { description: "Implement interface 'I2'" },
        { description: "Implement interface 'I3'", actions: [ { type: "IA", id: "IA1" } ] as Actions[] },
        { description: "Implement interface 'I4'", actions: [ { type: "IB", id: "IB1" } ] as Actions[], commands: [ "CMD1" ] }
    ] as VerifyCodeFixAvailableOptions[])
}