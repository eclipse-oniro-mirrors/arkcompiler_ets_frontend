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

const foo = async (): Promise<Double> => {
    return 42.0; // Return a double to satisfy the Promise<Double>
};

foo().then((result) => {                                     // crash - issue #26815
    console.log(`Async function returned: ${result}`);
});

let fooArray = ["a", "b", "c"];
let fooArrayMapped = fooArray.map(x => x + " is mapped");    // crash - issue #26856
let fooArrayMappedAndChecked = fooArray.map(x => x == "a");  // crash - issue #26856
