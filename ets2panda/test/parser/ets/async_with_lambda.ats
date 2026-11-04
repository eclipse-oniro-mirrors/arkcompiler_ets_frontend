/**
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

let global: int;

async function func(param: int): Promise<String | null> {
    let local: int;
    let lambda: () => void = (): void => {
        param;
        local;
        global;
        let x = 0;
    }
    let async_lambda: () => Promise<Object | null> = async (): Promise<Object | null> => {
        param;
        local;
        global;
        let x = 0;
        return null;
    }
    let asyncLambda: () => Promise<Object> = async (): Promise<Object> => {
        return new Promise<Object>((resolve: (value: Object) => void): void => {
            resolve(new Object());
        });
    }
    return null;
}
