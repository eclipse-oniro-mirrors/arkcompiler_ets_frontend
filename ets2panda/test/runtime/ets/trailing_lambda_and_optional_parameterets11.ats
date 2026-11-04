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

// Test scenarios with multiple lambda parameters
class B {
    // Mix required and optional parameters (non-lambda first)
    bar(requiredFn: () => void, optionalNum?: number, optionalFn?: () => string) {
        requiredFn();
        arktest.assertEQ(optionalNum, 42);
        if (optionalFn) arktest.assertEQ(optionalFn(), "optional");
    }

    // Multiple optional lambda parameters
    baz(a: number, f1?: () => string, f2?: () => number) {
        arktest.assertEQ(a, 10);
        if (f1) arktest.assertEQ(f1(), "first");
        if (f2) arktest.assertEQ(f2(), 42);
    }
}

function main() {
    const b = new B();

    b.bar(() => { }, 42, () => "optional");

    b.bar(() => { }, 42) {
        return "optional";
    };

    b.baz(10, () => "first", () => 42);

    b.baz(10, () => "first"){
        return 42
    }
}