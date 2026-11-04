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

class cls {}

let b : cls | null = null;
let c : cls = new cls();

function foo(arg: cls | null) : void {
}

function main(): void {
    foo(b);
    foo(c);

    let e : cls | null = null;
    let f : cls = new cls();

    foo(e);
    foo(f);

    foo(null);
    foo(new cls());

    arktest.assertEQ(b, null);
    arktest.assertNE(c, null);
    arktest.assertNE(null, c);
    arktest.assertEQ(e, null);
    arktest.assertNE(f, null);
    arktest.assertEQ(b, e);
    arktest.assertEQ(null, null);

    return;
}
