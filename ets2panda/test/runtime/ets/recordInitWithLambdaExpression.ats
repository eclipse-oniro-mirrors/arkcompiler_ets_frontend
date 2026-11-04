/*
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
type HandlerMap = Record<int, (param: Number) => string>

function main(): void {
    let handlerMap: HandlerMap = {
        2: (param: Number):string => { return "1" },
        4: (param: Number):string => { return "2" },
        8: (param: Number):string => { return "3" },
        16: (param: Number):string => { return "4" },
        32: (param: Number):string => { return "5" },
        64: (param: Number):string => { return "6" },
        128: (param: Number):string => { return "7" },
        256: (param: Number):string => { return "8" }
    }
    arktest.assertEQ(handlerMap.get(2)!(1), "1")
    arktest.assertEQ(handlerMap.get(4)!(1), "2")
    arktest.assertEQ(handlerMap.get(8)!(1), "3")
    arktest.assertEQ(handlerMap.get(16)!(1), "4")
    arktest.assertEQ(handlerMap.get(32)!(1), "5")
    arktest.assertEQ(handlerMap.get(64)!(1), "6")
    arktest.assertEQ(handlerMap.get(128)!(1), "7")
    arktest.assertEQ(handlerMap.get(256)!(1), "8")
}
