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

function fooAnd(x: String|null, y: String|null): string {
  if (x != null && y != null)  {
    return x + " " + y;
  } else if (x == null && y == null) {
    return "null";
  } else if (x != null && y == null) {
    return x;
  } else if (x == null && y != null) {
     return y;
  } else {
     throw new Error("Unreachable");
  }
}

function fooOr1(x: String|null, y: String|null): string {
  if (x != null || y != null)  {
    return "case 1";
  } else if (x == null && y == null) {
    return "null";
  } else {
     throw new Error("Unreachable");
  }
}

function fooOr2(x: String|null, y: String|null): string {
  if (x == null || y == null)  {
    return "case 1";
  } else if (x != null && y != null) {
    return x + " " + y;
  } else {
     throw new Error("Unreachable");
  }
}

function main(): void {
  arktest.assertEQ(fooAnd("Test", "string"), "Test string")
  arktest.assertEQ(fooAnd("Test", null), "Test")
  arktest.assertEQ(fooAnd(null, "string"), "string")
  arktest.assertEQ(fooAnd(null, null), "null")

  arktest.assertEQ(fooOr1("Test", "string"), "case 1")
  arktest.assertEQ(fooOr1("Test", null), "case 1")
  arktest.assertEQ(fooOr1(null, "string"), "case 1")
  arktest.assertEQ(fooOr1(null, null), "null")

  arktest.assertEQ(fooOr2("Test", "string"), "Test string")
  arktest.assertEQ(fooOr2("Test", null), "case 1")
  arktest.assertEQ(fooOr2(null, "string"), "case 1")
  arktest.assertEQ(fooOr2(null, null), "case 1")
}
