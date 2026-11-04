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

function labeledSwitchTest01(): void {
    let value: int = 10;
    let result: int;

    label1:
    for (let i = 0; i < 10; i++) {
        switch (value) {
            case 0:
            case 1:
                result = 1;
                break;
            case 2:
                result = 2;
                continue label1;
            default:
                result = 3400;
        }
    }
}

function labeledSwitchTest02(): void {
    let value: int = 10;
    let result: int;

    for (let i = 0; i < 10; i++) {
        label1:
        switch (value) {
            case 0:
            case 1:
                result = 1;
                break;
            case 2:
                result = 2;
                break label1;
            default:
                result = 3400;
        }
    }
}
