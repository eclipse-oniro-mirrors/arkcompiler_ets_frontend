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

// A bare string literal in a destructuring initializer must be widened to `string` (not kept as the
// CONSTANT literal type `"str"`), so the destructured element can be reassigned any string value.
// The same holds for `as string` casts and for number/boolean literals.
function main(): void {
    // Bare string literal: element type must be `string`, not `"str"`.
    let s: string = "a";
    let ns = [s] = ["str"];
    arktest.assertEQ(ns[0], "str");
    ns[0] = "new str";
    arktest.assertEQ(ns[0], "new str");

    // `as string` cast: same widened `string` element type.
    let s2: string = "a";
    let ns2 = [s2] = ["str" as string];
    arktest.assertEQ(ns2[0], "str");
    ns2[0] = "new str";
    arktest.assertEQ(ns2[0], "new str");

    // Number literal: element type must be `int`, reassignable.
    let n: int = 1;
    let nn = [n] = [2];
    arktest.assertEQ(nn[0], 2);
    nn[0] = 3;
    arktest.assertEQ(nn[0], 3);

    // Boolean literal: element type must be `boolean`, reassignable.
    let b: boolean = true;
    let nb = [b] = [false];
    arktest.assertEQ(nb[0], false);
    nb[0] = true;
    arktest.assertEQ(nb[0], true);
}
