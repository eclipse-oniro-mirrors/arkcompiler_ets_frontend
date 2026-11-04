/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

// BigInt-vs-numeric equality comparisons (==, !=, ===, !==) are valid.
// BigInt-vs-numeric relational comparisons (>, <, >=, <=) are compile-time errors.

function basicComparison() {
    arktest.assertTrue(1n == 1n);
    arktest.assertTrue((1n as Any) == (1n as Any));
    arktest.assertFalse(1n == 0n);
    arktest.assertFalse(1n == 1);
}

function numberMaxSafeIntComp() {
    const MAX_SAFE = Number.MAX_SAFE_INTEGER;
    arktest.assertTrue(BigInt(MAX_SAFE) != MAX_SAFE);
    arktest.assertTrue(BigInt(MAX_SAFE) == BigInt(MAX_SAFE));
    arktest.assertTrue(BigInt(MAX_SAFE).doubleValue() == MAX_SAFE);
}

function numberMinSafeIntComp() {
    const MIN_SAFE = Number.MIN_SAFE_INTEGER;
    arktest.assertTrue(BigInt(MIN_SAFE) != MIN_SAFE);
    arktest.assertTrue(BigInt(MIN_SAFE) == BigInt(MIN_SAFE));
    arktest.assertTrue(BigInt(MIN_SAFE).doubleValue() == MIN_SAFE);
}

function nanComp() {
    arktest.assertFalse(1n == NaN);
}

function zeroValueComp() {
    arktest.assertTrue(0n != 0);
    arktest.assertTrue(0n == 0n);
    arktest.assertTrue((0n).doubleValue() == 0);
}

function negativeComp() {
    arktest.assertTrue(-1n != -1);
    arktest.assertTrue(-1n == -1n);
    arktest.assertTrue((-1n).doubleValue() == -1);
}

function moreBoundaryTests() {
    arktest.assertTrue(Number.MAX_SAFE_INTEGER + 1 === Number.MAX_SAFE_INTEGER + 2);
    arktest.assertTrue(BigInt(Number.MAX_SAFE_INTEGER + 1) === BigInt(Number.MAX_SAFE_INTEGER + 2));
}

function strictEqualityTests() {
    arktest.assertTrue(1n !== 1);
    arktest.assertTrue((1n).doubleValue() === 1);
    arktest.assertTrue(0n !== 0);
    arktest.assertTrue((0n).doubleValue() === 0);
    arktest.assertTrue(-1n !== -1);
    arktest.assertTrue((-1n).doubleValue() === -1);
}

function looseEqualityTests() {
    arktest.assertTrue(1n != 1);
    arktest.assertTrue((1n).doubleValue() == 1);
    arktest.assertTrue(0n != 0);
    arktest.assertTrue((0n).doubleValue() == 0);
    arktest.assertTrue(-1n != -1);
    arktest.assertTrue((-1n).doubleValue() == -1);
}

function main() {
    basicComparison();
    numberMaxSafeIntComp();
    numberMinSafeIntComp();
    nanComp();
    zeroValueComp();
    negativeComp();
    moreBoundaryTests();
    strictEqualityTests();
    looseEqualityTests();
}