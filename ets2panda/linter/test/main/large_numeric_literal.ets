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

 Test large number literal rule
// These should be detected as errors

// Large number literals beyond 2^63 - 1 to -2^63 range
const tooLargePositive = 9223372036854775808;  // Error: exceeds 2^63 - 1
const tooLargeNegative = -9223372036854775809;  // Error: below -2^63

// Edge cases - these should not report errors
const maxAllowedPositive = 9223372036854775807;  // Correct: exactly 2^63 - 1
const maxAllowedNegative = -9223372036854775808;  // Correct: exactly -2^63

// Normal numbers - these should not report errors
const normalPositive = 1000000;
const normalNegative = -1000000;
const zero = 0;

// Large but acceptable numbers
const largeButOk = 9223372036854775800;  // Correct: within range
const largeButOkNegative = -9223372036854775800;  // Correct: within range

// Function parameters
function processLargeNumber(
  value: number = 9223372036854775808  // Error: exceeds 2^63 - 1
): void {
  console.log(value);
}

// Interface property
interface DataConfig {
  maxValue: number;  // This will be checked when assigned
}

// Variable assignment
let config: DataConfig = {
  maxValue: 9223372036854775808  // Error: exceeds 2^63 - 1
};

// Array literal
const largeNumbers = [
  9223372036854775808,  // Error: exceeds 2^63 - 1
  -9223372036854775809,  // Error: below -2^63
  1000,  // Correct: normal number
  9223372036854775807   // Correct: exactly at limit
];

// Object literal
const settings = {
  threshold: 9223372036854775808,  // Error: exceeds 2^63 - 1
  limit: 9223372036854775807       // Correct: exactly at limit
}; 