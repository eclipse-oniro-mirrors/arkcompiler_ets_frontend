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

 class A {
    status: int = 0;
    first: int = 0;
    second: int = 0;

    errorAlert(): void {
        this.status = 2;
    }

    incFirst(): void {
        this.first++;
    }

    incSecond(): void {
        this.second++;
    }

    run(): int {
        switch (5) {
            default:
                this.incFirst();
                this.incSecond();
                break;
            case 2:
                this.errorAlert();
                break;
        }
        if (this.first !== 1) this.errorAlert();
        if (this.second !== 1) this.errorAlert();
        return this.status;
    }
}

function main() {
    arktest.assertEQ(new A().run(), 0)
}