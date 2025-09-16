/**
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
 
import * as np from "./test_ets2ts_isolated_enum.ets"
import {B, C0 as C} from "./test_ets2ts_isolated_enum.ets"

export let b: np.A = new np.A

export default let d: int = 1

export function foo(a: int, c: C) 
{
    if (a < 0) {
        return 1;
    }
    if (a == 0) {
        return b;
    }
    if (a == 1) {
        return new B;
    }
    if (a == 2) {
        return ["hello", "arkts"];
    }
    if (a == 3) {
        return [0, 1, 2];
    }
    if (a == 4) {
        return [true, false, false, true];
    }
    
    return "hello";
}