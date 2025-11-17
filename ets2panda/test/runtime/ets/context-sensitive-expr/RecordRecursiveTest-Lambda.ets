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


type RecordData = undefined | Record<string, RecordData> | number | (() => void)

let callTrace: string = '';
let want: RecordData = {
    "recursive_lambda1": () => {
                                callTrace += "recursive_lambda1";
                                let want_in: RecordData = {
                                    "recursive_lambda1_1": () => {callTrace += " -> recursive_lambda1_1"}
                                };
                                ((want_in as Record<string, RecordData>)["recursive_lambda1_1"] as (() => void))();
                              },
    "recursive_lambda2": () => {let want_in: RecordData = {"int": 1}},
};

((want as Record<string, RecordData>)["recursive_lambda1"] as (() => void))();
arktest.assertEQ(callTrace, "recursive_lambda1 -> recursive_lambda1_1");
