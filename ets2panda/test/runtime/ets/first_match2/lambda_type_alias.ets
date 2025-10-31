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

type AsyncCallback<T> = (err: T, data: T) => void;
type AsyncCallback2 = (err: string, data: number, data2: number) => void;

function getBundleInfo(userId?: number): string {
    return "invoke1"
}

function getBundleInfo(callback: AsyncCallback<number>): string {
    callback(666, 66);
    return "invoke2"
}

function getBundleInfo(callback: AsyncCallback2): string {
    callback("invoke3", 66, 66);
    return "invoke3"
}

function main() {
    arktest.assertEQ(getBundleInfo((err, data) => {
        arktest.assertEQ(err, 666);
        arktest.assertEQ(data, 66)
    }), "invoke2")

    arktest.assertEQ(getBundleInfo((err: string, data1: number, data2: number) => {
        arktest.assertEQ(err, "invoke3");
        arktest.assertEQ(data1, 66);
        arktest.assertEQ(data2, 66);
    }), "invoke3")
}
