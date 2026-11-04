/*
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

function test_unary_minus(): void {
    arktest.assertEQ(new BigInt(10).negate(), (-10n))
    arktest.assertEQ(new BigInt(1000).negate(), (-1000n))
    arktest.assertEQ(new BigInt(-1).negate(), (1n))
    arktest.assertEQ(new BigInt(-10).negate(), (10n))
    arktest.assertEQ(new BigInt(-100).negate(), (100n))
    arktest.assertEQ(new BigInt(0).negate(), (0n))
}

function test_unary(): void {
    let n = -128n
    arktest.assertEQ(n, -128n)

    let a = 123n;
    arktest.assertEQ(a, 123n)
    arktest.assertEQ(a.toString(), "123")

    a = -123n;
    arktest.assertEQ(a, -123n)
    arktest.assertEQ(a.toString(), "-123")

    a = 123n
    arktest.assertEQ(a, 123n)
    arktest.assertEQ(a.toString(), "123")

    a = -a;
    arktest.assertEQ(a, -123n)
    arktest.assertEQ(a.toString(), "-123")

    a = -a;
    arktest.assertEQ(a, 123n)
    arktest.assertEQ(a.toString(), "123")

    let e: bigint;
    arktest.assertEQ(a, 123n)
    arktest.assertEQ(a.toString(), "123")
    e = -a;

    // Check that original value does not change
    arktest.assertEQ(a, 123n)
    arktest.assertEQ(a.toString(), "123")
    arktest.assertEQ(e, -123n)
    arktest.assertEQ(e.toString(), "-123")
    e = a;
    arktest.assertEQ(e, 123n)
    arktest.assertEQ(e.toString(), "123")

    let b = -123n;
    arktest.assertEQ(b, -123n)
    arktest.assertEQ(b.toString(), "-123")

    let c = -(123n);
    arktest.assertEQ(c, -123n)
    arktest.assertEQ(c.toString(), "-123")

    // Double unary minus
    let d = -(-123n);
    arktest.assertEQ(d, 123n)
    arktest.assertEQ(d.toString(), "123")

    // Triple unary minux
    let f = -(-(-123n));
    arktest.assertEQ(f, -123n)
    arktest.assertEQ(f.toString(), "-123")

    a = new BigInt(321);
    arktest.assertEQ(a.toString(), "321")
    arktest.assertEQ(a, 321n)

    b = -a;
    arktest.assertEQ(a.toString(), "321")
    arktest.assertEQ(a, 321n)
    arktest.assertEQ(b, -321n)
    arktest.assertEQ(b.toString(), "-321")
}

function main() : void {
    test_unary()
    test_unary_minus()
}
