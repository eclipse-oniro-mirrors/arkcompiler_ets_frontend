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

namespace OuterNamespace {
    export function outerFunction() {
        return "Outer function";
    }

    export namespace InnerNamespace {
        export function innerFunction() {
            return "Inner function";
        }

        export let sharedValue = 50;

        // Utility function to reset the state of InnerNamespace
        export function resetState() {
            sharedValue = 50;  // Reset sharedValue to its initial state
        }
    }

    // Utility function to reset the state of OuterNamespace
    export function resetOuterState() {
        InnerNamespace.resetState();
    }
}

function main() {
    // Reset OuterNamespace and InnerNamespace state before running tests
    OuterNamespace.resetOuterState();

    // Test access to outer and inner namespace members
    arktest.assertEQ(OuterNamespace.outerFunction(), "Outer function",  "Outer function failed")
    arktest.assertEQ(OuterNamespace.InnerNamespace.innerFunction(), "Inner function",  "Inner function failed")

    // Test modification of shared value in inner namespace
    OuterNamespace.InnerNamespace.sharedValue = OuterNamespace.InnerNamespace.sharedValue + 25;
    arktest.assertEQ(OuterNamespace.InnerNamespace.sharedValue, 75,  "Shared value update failed")
}