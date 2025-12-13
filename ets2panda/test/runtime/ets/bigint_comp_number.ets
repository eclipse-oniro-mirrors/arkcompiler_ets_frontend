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

function basicComparison() {
    arktest.assertTrue(1n < 2);
    arktest.assertTrue(2n > 1);
    arktest.assertTrue(1n == 1);
    arktest.assertTrue(1n <= 1.5);
    arktest.assertTrue(1n >= 0.5);
}

function floatPointComp() {
    arktest.assertTrue(1n < 1.2);
    arktest.assertFalse(1n > 1.2);
    arktest.assertFalse(2n < 1.9);
    arktest.assertTrue(2n > 1.9);
    arktest.assertTrue(0n < 0.1);
    arktest.assertTrue(0n > -0.1);
}

function numberMaxSafeIntComp() {
    const MAX_SAFE = Number.MAX_SAFE_INTEGER;
    arktest.assertFalse(BigInt(MAX_SAFE) < MAX_SAFE);
    arktest.assertTrue(BigInt(MAX_SAFE) <= MAX_SAFE);
    arktest.assertFalse(BigInt(MAX_SAFE) > MAX_SAFE);
    arktest.assertTrue(BigInt(MAX_SAFE) >= MAX_SAFE);
    arktest.assertTrue(BigInt(MAX_SAFE) == MAX_SAFE);
    arktest.assertTrue(BigInt(MAX_SAFE - 1) < MAX_SAFE);
    arktest.assertTrue(BigInt(MAX_SAFE + 1) > MAX_SAFE);
}

function numberMinSafeIntComp() {
    const MIN_SAFE = Number.MIN_SAFE_INTEGER;
    arktest.assertFalse(BigInt(MIN_SAFE) < MIN_SAFE);
    arktest.assertTrue(BigInt(MIN_SAFE) <= MIN_SAFE);
    arktest.assertFalse(BigInt(MIN_SAFE) > MIN_SAFE);
    arktest.assertTrue(BigInt(MIN_SAFE) >= MIN_SAFE);
    arktest.assertTrue(BigInt(MIN_SAFE) == MIN_SAFE);
    arktest.assertTrue(BigInt(MIN_SAFE - 1) < MIN_SAFE);
    arktest.assertTrue(BigInt(MIN_SAFE + 1) > MIN_SAFE);
}

function numberMaxValueComp() {
    const MAX_VALUE = Number.MAX_VALUE;
    arktest.assertTrue(1n < MAX_VALUE);
    arktest.assertTrue(0n < MAX_VALUE);
    arktest.assertTrue(-1n < MAX_VALUE);
}

function largeBigIntComp() {
    const MAX_VALUE = Number.MAX_VALUE;
    const largeBigInt = 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000n;
    arktest.assertTrue(largeBigInt < MAX_VALUE);
}

function numberMinValueComp() {
    const MIN_VALUE = Number.MIN_VALUE;
    arktest.assertTrue(0n < MIN_VALUE);
    arktest.assertFalse(0n > MIN_VALUE);
    arktest.assertFalse(1n < MIN_VALUE);
    arktest.assertTrue(1n > MIN_VALUE);
    arktest.assertTrue(-1n < MIN_VALUE);
}

function infinityComp() {
    arktest.assertTrue(1n < Infinity);
    arktest.assertFalse(1n > Infinity);
    arktest.assertTrue(1n <= Infinity);
    arktest.assertFalse(1n >= Infinity);
    arktest.assertTrue(-1n < Infinity);
    arktest.assertFalse(-1n > Infinity);
    arktest.assertTrue(-1n <= Infinity);
    arktest.assertFalse(-1n >= Infinity);
    arktest.assertFalse(1n < -Infinity);
    arktest.assertTrue(1n > -Infinity);
    arktest.assertFalse(1n <= -Infinity);
    arktest.assertTrue(1n >= -Infinity);
}

function nanComp() {
    arktest.assertFalse(1n < NaN);
    arktest.assertFalse(1n > NaN);
    arktest.assertFalse(1n <= NaN);
    arktest.assertFalse(1n >= NaN);
    arktest.assertFalse(1n == NaN);
}

function zeroValueComp() {
    arktest.assertFalse(0n < 0);
    arktest.assertFalse(0n > 0);
    arktest.assertTrue(0n <= 0);
    arktest.assertTrue(0n >= 0);
    arktest.assertTrue(0n == 0);
    arktest.assertFalse(0n < -0);
    arktest.assertFalse(0n > -0);
    arktest.assertTrue(0n <= -0);
    arktest.assertTrue(0n >= -0);
}

function negativeComp() {
    arktest.assertFalse(-1n < -1);
    arktest.assertTrue(-1n <= -1);
    arktest.assertFalse(-1n > -1);
    arktest.assertTrue(-1n >= -1);
    arktest.assertTrue(-1n == -1);
    arktest.assertTrue(-1n < -0.5);
    arktest.assertTrue(-1n > -1.5);
}

function veryLargeBigIntComp() {
    const veryLargeBigInt = 1234567890123456789012345678901234567890n;
    arktest.assertTrue(veryLargeBigInt < Infinity);
    arktest.assertTrue(veryLargeBigInt > Number.MAX_SAFE_INTEGER);
}

function verySmallBigIntComp() {
    const verySmallBigInt = -1234567890123456789012345678901234567890n;
    arktest.assertTrue(verySmallBigInt > -Infinity);
    arktest.assertTrue(verySmallBigInt < Number.MIN_SAFE_INTEGER);
}

function precisionLossComp() {
    // This loss precision in TS, but not in ArkTS
    const precisionTest1 = 9007199254740992n;
    const precisionTest2 = 9007199254740992;
    arktest.assertFalse(precisionTest1 < precisionTest2);
    arktest.assertTrue(precisionTest1 <= precisionTest2);
    arktest.assertFalse(precisionTest1 > precisionTest2);
    arktest.assertTrue(precisionTest1 >= precisionTest2);
}

function unsafeRangeComp() {
    // This loss precision in TS, but not in ArkTS
    const unsafeBig1 = 9007199254740993n;
    const unsafeNum1 = 9007199254740993;
    arktest.assertFalse(unsafeBig1 < unsafeNum1);
    arktest.assertFalse(unsafeBig1 > unsafeNum1);
    arktest.assertTrue(unsafeBig1 <= unsafeNum1);
    arktest.assertTrue(unsafeBig1 >= unsafeNum1);
}

function moreBoundaryTests() {
    arktest.assertTrue(Number.MAX_SAFE_INTEGER + 1 === Number.MAX_SAFE_INTEGER + 2);
    arktest.assertTrue(BigInt(Number.MAX_SAFE_INTEGER + 1) === BigInt(Number.MAX_SAFE_INTEGER + 2));
}

function strictEqualityTests() {
    arktest.assertTrue(1n === 1);
    arktest.assertTrue(0n === 0);
    arktest.assertTrue(-1n === -1);
}

function looseEqualityTests() {
    arktest.assertTrue(1n == 1);
    arktest.assertTrue(0n == 0);
    arktest.assertTrue(-1n == -1);
}

function main() {
    basicComparison();
    floatPointComp();
    numberMaxSafeIntComp();
    numberMinSafeIntComp();
    numberMaxValueComp();
    largeBigIntComp();
    numberMinValueComp();
    infinityComp();
    nanComp();
    zeroValueComp();
    negativeComp();
    veryLargeBigIntComp();
    verySmallBigIntComp();
    precisionLossComp();
    unsafeRangeComp();
    moreBoundaryTests();
    strictEqualityTests();
    looseEqualityTests();
}
