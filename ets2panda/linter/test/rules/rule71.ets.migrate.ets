/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

for (let i = 0, j = 0; i < 10; ++i, j += 2) {
    console.log(i)
    console.log(j)
}

let x = 0

// Error, no autofix
x = (++x, x++)

// No error
for (let i = 0, j = 0; i < 10; ++i, j += 2) {
    console.log(i)
    console.log(j)
}

let x2 = 0
++x2
x2 = x2++

let c = () => 33;

// Error, no autofix
const a = (1, b = 2, c());

// Error, no autofix
const r = (c(), b, 1)

class Test {
    // Error, no autofix
    static readonly sr = (1, c(), 2);

    // Error, no autofix
    field1 = (1, 2, c());

    method() {
        // Error, no autofix
        this.field1 = (c(), sr, 1);
    }
}

// Error, autofix
x++;
 x--;

// Error, autofix
x++, x--, ++x, --x, x

// Error, no autofix
if (x++, x === 1) {
    // Error, autofix
    x++;
 x--;
 ++x;
 --x;
 x;
}

// Error, autofix
x++ + x--;
 ++x;
 --x;

// Error, autofix
++x, x-- + x++, --x

// Error, autofix
++x;
 --x;
 x-- + x++;

// Error, autofix
x === x, --x, x === x, x++, x === x

// Error, autofix
x instanceof number;
 --x;
 x instanceof number;
 x++;
 x instanceof number;

// Error, autofix
x in x, --x, x in x, x++, x in x
