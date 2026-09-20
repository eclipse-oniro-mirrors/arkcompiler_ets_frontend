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

interface PlainRoArray {
    input: readonly number[];
}

interface GenericRoArray {
    input: readonly Array<number>;
}

interface ReadonlyTuple {
    input: readonly [number, string];
}

interface NestedRoArray {
    inner: PlainRoArray;
}

function build(): PlainRoArray {
    return {
        input: [1, 2, 3]
    };
}

function main(): void {
    let plain: PlainRoArray = { input: [1, 2, 3] };
    arktest.assertEQ(plain.input.length, 3);
    arktest.assertEQ(plain.input[0], 1.0);
    arktest.assertEQ(plain.input[2], 3.0);

    let generic: GenericRoArray = { input: [4, 5, 6] };
    arktest.assertEQ(generic.input.length, 3);
    arktest.assertEQ(generic.input[1], 5.0);

    let tuple: ReadonlyTuple = { input: [7, 'x'] };
    arktest.assertEQ(tuple.input[0], 7.0);
    arktest.assertEQ(tuple.input[1], 'x');

    let nested: NestedRoArray = { inner: { input: [8, 9] } };
    arktest.assertEQ(nested.inner.input[0], 8.0);
    arktest.assertEQ(nested.inner.input[1], 9.0);

    let rows: Array<PlainRoArray> = [ { input: [1] }, { input: [2, 3] } ];
    arktest.assertEQ(rows.length, 2);
    arktest.assertEQ(rows[0].input[0], 1.0);
    arktest.assertEQ(rows[1].input[1], 3.0);

    let viaFn = build();
    arktest.assertEQ(viaFn.input[1], 2.0);
}