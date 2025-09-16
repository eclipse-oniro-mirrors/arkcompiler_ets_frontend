/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function eq(x: Object | null | undefined, y: Object | null | undefined) {
    return x == y;
}

function box<T extends Object>(v: T): Object { return v; }

// NOTE(vpukhov): char included?
function test_numerics() {
    let v: byte = 42;
    const create_set = () =>
        [box<byte>(v), box<short>(v), box<int>(v), box<long>(v),
        box<float>(v), box<double>(v)];

    let arr1 = create_set();
    let arr2 = create_set();

    for (const e1 of arr1) {
        for (const e2 of arr2) {
            arktest.assertTrue(eq(e1, e2))
        }
    }
}

// NOTE(vpukhov): the same is applied to float?
// NOTE(vpukhov): char == int overlow?
function test_numeric_precision() {
    const maxsafe = Double.toLong(Double.MAX_SAFE_INTEGER)
    arktest.assertTrue(!eq(maxsafe * 4 + 1, maxsafe * 4))
    arktest.assertTrue(eq(maxsafe * 4 + 1, Long.toDouble(maxsafe * 4)))
}

function main() {
    arktest.assertTrue(eq(null, null));
    arktest.assertTrue(eq(null, undefined));
    arktest.assertTrue(eq(undefined, null));
    arktest.assertTrue(eq(undefined, undefined));
    arktest.assertTrue(eq("abc", ((a: string, b: string): string => { return a + b })("a", "bc")));
    arktest.assertTrue(eq(123n, ((a: bigint, b: bigint): bigint => { return a + b })(120n, 3n)));
    arktest.assertTrue(eq(true, true));
    arktest.assertTrue(eq(box<int>(123), box<int>(123)));
    arktest.assertTrue(eq('a', 'a'));

    arktest.assertTrue(!eq(null, false))
    arktest.assertTrue(!eq(undefined, false))
    arktest.assertTrue(!eq(null, 0))
    arktest.assertTrue(!eq(true, false))
    arktest.assertTrue(!eq("", 0))
    arktest.assertTrue(!eq("", false))
    arktest.assertTrue(!eq("abc", "cde"))
    arktest.assertTrue(!eq("1", 1))
    arktest.assertTrue(!eq(123n, 124n))
    arktest.assertTrue(!eq(-123n, 123n))
    arktest.assertTrue(!eq('a', 'b'))
    arktest.assertTrue(!eq(box<int>(1), box<int>(2)))

    test_numerics();
    test_numeric_precision();
}
