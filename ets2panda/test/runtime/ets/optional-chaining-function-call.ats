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

type funcType = () => String;

function getMelon(): String { return "melon" }

function dummy(): funcType | null {
  return (() => { return "peach"; }) as funcType; // #15577
}

function foo(a: funcType | null) {
    let fruit: String = a?.() ?? "apple";

    let getPeach: funcType | null = dummy();
    fruit = getPeach?.() ?? "banana";
    arktest.assertEQ(fruit, "peach")

    fruit = getMelon() ?? "strawberry";
    arktest.assertEQ(fruit, "melon")

    let test = false;
    try {
        fruit = a!();
    } catch (e) {
        if(e instanceof NullPointerError) {
            test = true;
        }
    }

    arktest.assertEQ(test, true)
}

function main(): void {
    let getFruit = null;
    foo(getFruit);

    let getPeach: funcType | null = dummy();
    let b = getPeach?.();
    arktest.assertEQ(b, "peach")
}
