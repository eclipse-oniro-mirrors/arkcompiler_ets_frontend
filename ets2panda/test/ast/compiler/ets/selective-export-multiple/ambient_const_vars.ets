/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

// this file is imported and used by `./import_ambient_const_neg.ets`

export declare const double_1: double;
declare const double_2: double, double_3: double;
export { double_3 }

let double_4 = 1.0;
let double_5 = 9007199254740991.0;
export { double_5 }
/* @@? 23:5 Warning Warning W71336: Exported variable 'double_5' requires type annotation. */