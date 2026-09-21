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

// "" is a valid property key and also the name a non-method member answers
// with: the field must fail the eligibility check instead of being miscast.
// The field itself stores nothing on the prototype here.

class NoMergeEmptyKeyField {
    get ""() { return 1 }
    f = 1;
    set ""(v) { }
};
const d = Object.getOwnPropertyDescriptor(NoMergeEmptyKeyField.prototype, "");
print(typeof d.get, typeof d.set, typeof d.value);
