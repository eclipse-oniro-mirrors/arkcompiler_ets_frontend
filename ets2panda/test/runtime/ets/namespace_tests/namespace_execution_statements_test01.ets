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

namespace ComprehensiveNamespace {
    // 1. Variable assignment
    export let counter = 0;
    export let message = "Initial message";

    // 2. Assignment expression
    counter = counter + 5;

    // 3. Function declaration and call
    export function incrementCounter() {
        counter++;
    }

    export function resetMessage() {
        message = "Message reset inside namespace";
    }

    // 4. Conditional statement
    export function checkCounter() {
        if (counter > 5) {
            message = "Counter is greater than 5";
        } else {
            message = "Counter is less than or equal to 5";
        }
    }

    // 5. Loop statement
    export function loopTest() {
        let sum = 0;
        for (let i = 1; i <= 5; i++) {
            sum += i;
        }
        return sum;
    }

    // Utility function to reset the state of the namespace
    export function resetState() {
        counter = 0;
        message = "Initial message";
        counter = counter + 5; // Reapply the assignment expression
    }
}

function main() {
    // Reset namespace state before running tests
    ComprehensiveNamespace.resetState();

    // 1. Test variable assignment
    arktest.assertEQ(ComprehensiveNamespace.counter, 5,  "Counter should be 5 after assignment")

    // 2. Test assignment expression
    arktest.assertEQ(ComprehensiveNamespace.message, "Initial message",  "Message should be the initial value")

    // 3. Test function call
    ComprehensiveNamespace.incrementCounter();
    arktest.assertEQ(ComprehensiveNamespace.counter, 6,  "Counter should be 6 after increment")

    ComprehensiveNamespace.resetMessage();
    arktest.assertEQ(ComprehensiveNamespace.message, "Message reset inside namespace",  "Message should be reset after function call")

    // 4. Test conditional statement
    ComprehensiveNamespace.checkCounter();
    arktest.assertEQ(ComprehensiveNamespace.message, "Counter is greater than 5",  "Condition failed for counter > 5")

    // 5. Test loop statement
    let loopResult = ComprehensiveNamespace.loopTest();
    arktest.assertEQ(loopResult, 15,  "Loop test failed, sum should be 15")
}


