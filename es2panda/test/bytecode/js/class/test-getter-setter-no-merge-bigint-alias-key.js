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

// The alias trap: ToString(1n) === "1", so a BigInt data property keyed like
// a numeric pair must keep the scan stopped - skipping it would let the
// merged pair jump before the data definition and flip the final property
// from master's set-only accessor to data 5.

const aliasTrap = {
    get 1() { return 1 },
    1n: 5,
    set 1(v) { }
};
