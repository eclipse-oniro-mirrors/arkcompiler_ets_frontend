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

function foo(f: () => int): int {
    return f()
}

// AstNodes inside the trailing lambda are cloned - check that Clone works
let a = foo() {
    for (let i = 0; i < 5; i = i + 1) {
        break
    }
    let x = 0
    while (x < 5) {
        x = x + 1
        continue
    }
    a: do {
        x = x - 1
        continue a
        break a
    } while (x > 0)

    switch (x) {
        case 1:
        break
    }
    return 1
}
