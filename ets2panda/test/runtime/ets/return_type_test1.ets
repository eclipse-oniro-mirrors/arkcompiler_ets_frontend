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

export type AsyncCallback = (err: Error | null, data: number | undefined) => void;
function createSubscriber(callback: AsyncCallback): void { };

class XX {
    async aaa() {
        let pro = new Promise<void>((resolve) => {
            resolve(undefined)
        })
        const thenResult = pro.then(async (): Promise<void> => {
            createSubscriber((err, data): void => {
                return;
            })
        })
        return thenResult;
    }
}

function main() {
    let xx = new XX();

    xx.aaa().then(result => {
        arktest.assertTrue(result === undefined);
    });
}
