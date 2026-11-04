/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
function main() {
    let v = 0
    let f: () => void = () => {
        v = 1
        {
	   v = 2
        }
        if (true) {
            v = 3
        }
        for (let x: int of [0])
            v = 4

/* TODO(gogabr): embedding of lambdas is not supported yet.
        let g: () => void = () => {
             v = 5
        }
        g()
*/
    }
    f()
    console.log(v)
}
