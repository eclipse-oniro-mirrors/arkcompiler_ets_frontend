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

// Test optional tuple type rule

// Basic optional tuple type
type BasicOptionalTuple = [string, number?];  // Error: number? is optional

// Optional element in nested tuple
type NestedOptionalTuple = [string, [number, string?]];  // Error: string? is optional

// Optional tuple type in function parameters
function processOptionalTuple(
  data: [string, number?]  // Error: number? is optional
): void {
  console.log(data);
}

// Optional tuple property in interface
interface DataProcessor {
  process(data: [string, number?]): void;  // Error: number? is optional
}

// Optional tuple in type
let t: [number, boolean?] = [1];  // Error: boolean? is optional

// Correct usage
type CorrectTuple1 = [string, number];  // Correct: all elements are required

// Correct function parameter usage
function processCorrectTuple(
  data: [string, number]  // Correct: all elements are required
): void {
  console.log(data);
}

// Correct interface usage
interface CorrectDataProcessor {
  process(data: [string, number]): void;  // Correct: all elements are required
}

// Correct type alias usage
type CorrectDataTuple = [string, number];  // Correct: all elements are required 