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

interface IBalance {
    balance: int
}

class BalanceImpl implements IBalance {
    balance: int = 100
}

function main(): void {
    let i: IBalance = new BalanceImpl();
    arktest.assertEQ(i.balance, 100);

    i.balance = 250;
    arktest.assertEQ(i.balance, 250);

    // Ensure the underlying class field is updated as well.
    let impl = i as BalanceImpl;
    arktest.assertEQ(impl.balance, 250);
}
