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

function main(): void {
    // Constant-folded exponentiation: negative base with integral-valued real
    // exponent is well-defined per IEEE 754; 'x ** +/-0' is 1 for any 'x'.
    arktest.assertDoubleEQ((-0.5) ** 0.0, 1, 0.00001);
    arktest.assertDoubleEQ((-0.5) ** -0.0, 1, 0.00001);
    arktest.assertDoubleEQ((-2.0) ** 2.0, 4, 0.00001);
    arktest.assertDoubleEQ((-2.0) ** 3.0, -8, 0.00001);
    arktest.assertDoubleEQ((-2.0) ** -2.0, 0.25, 0.00001);
    arktest.assertDoubleEQ((-0.5f) ** 2.0f, 0.25, 0.00001);
    arktest.assertDoubleEQ((-2) ** 3, -8, 0.00001);
    arktest.assertDoubleEQ((0.0 / 0.0) ** 0.0, 1, 0.00001);
    arktest.assertEQ((-0.0) ** -1.0, -Infinity);

    // Negative base with a genuinely fractional exponent is NaN.
    arktest.assertTrue(isNaN((-0.5) ** 0.5));
    arktest.assertTrue(isNaN((-0.5f) ** 0.5f));

    // The same shapes computed at runtime must agree with the folded constants.
    let base: double = 0.5;
    let exp: double = 0.5;
    arktest.assertDoubleEQ((-base) ** 0.0, 1, 0.00001);
    arktest.assertDoubleEQ((-base) ** 2.0, 0.25, 0.00001);
    arktest.assertTrue(isNaN((-base) ** exp));
}
