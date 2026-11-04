/*
 * Copyright (c) 2023 - 2024-2025 Huawei Device Co., Ltd.
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

class C  {
  bar(): string {
    return "Class C";
  }
}

function foo(c: Object|null|undefined): string {
  if (c instanceof string)  {
    arktest.assertEQ(c.length, 11)
    c = "Case 1";
  } else if (c instanceof C) {
    arktest.assertEQ(c.bar(), "Class C")
    c = "Case 2";
  } else if (c instanceof Int) {
    arktest.assertEQ(c * 7, 49)
    c = "Case 3";
  } else if (c instanceof null) {
    arktest.assertEQ(c, null)
    c = "Case 4";
  } else {
    c = "Case 5";
  }

  arktest.assertEQ(c.length, 6)
  return c;
}


function main(): void {
  arktest.assertEQ(foo("Test string"), "Case 1")
  arktest.assertEQ(foo(new Int(7)), "Case 3")
  arktest.assertEQ(foo(new C()), "Case 2")
  arktest.assertEQ(foo(null), "Case 4")
  arktest.assertEQ(foo(undefined), "Case 5")
  arktest.assertEQ(foo(new Number(3.0)), "Case 5")
}
