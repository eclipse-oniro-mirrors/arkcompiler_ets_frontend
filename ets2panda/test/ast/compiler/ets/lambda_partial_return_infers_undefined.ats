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

function consume(callback: ((p: number) => boolean) | undefined): number {
    return 1;
}

function caseContextualCallArgument(): void {
    /* @@? 22:13 Error Semantic error ESE0104: Not all code paths return a value. */
    consume((e: number) => {
        if (e == 1) {
            return false;
        }
    });
}

function caseContextualVariable(): void {
    /* @@? 31:44 Error Semantic error ESE0104: Not all code paths return a value. */
    let callback: (p: number) => boolean = (e: number) => {
        if (e == 1) {
            return false;
        }
    };
}
