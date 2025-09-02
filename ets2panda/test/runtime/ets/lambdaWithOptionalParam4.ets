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

type lambdaType = (a?: int) => void;

function a(b: lambdaType): void {
    b();
    b(10);
}

function main(): void {
    let d : lambdaType = (c?: int) => {
        arktest.assertTrue(c == undefined || c == 10)
    }

    d();
    d(10);
    a(d);
}
