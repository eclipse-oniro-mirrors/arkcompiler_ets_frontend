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

// State tracker
class Tracker {
    static lambdaCalled = false;
    static blockExecuted = false;
    static reset() {
        Tracker.lambdaCalled = false;
        Tracker.blockExecuted = false;
    }
}

// Test class
class TestClass {
    // Method with mandatory lambda
    withMandatoryLambda(a: number, f: () => void) {
        f();
    }

    // Method with optional lambda
    withOptionalLambda(a?: number, f?: () => void) {
        f?.();
    }
}

// Test function
function testFunction(a: number, f: () => void) {
    f();
}

function main() {
    /* Scenario 1: Class method with mandatory parameter ​*/
    Tracker.reset();
    new TestClass().withMandatoryLambda(1, () => {
        Tracker.lambdaCalled = true;
    }) 
    {
        Tracker.blockExecuted = true;
    }
    arktest.assertTrue(Tracker.lambdaCalled, "Scenario 1: Lambda should be executed");
    arktest.assertTrue(Tracker.blockExecuted, "Scenario 1: Block should be executed");

    /* Scenario 2: Class method with optional parameter ​*/
    Tracker.reset();
    new TestClass().withOptionalLambda(1, () => {
        Tracker.lambdaCalled = true;
    }) 
    {
        Tracker.blockExecuted = true;
    }
    arktest.assertTrue(Tracker.lambdaCalled, "Scenario 2: Lambda should be executed");
    arktest.assertTrue(Tracker.blockExecuted, "Scenario 2: Block should be executed");

    /* Scenario 3: Function call with mandatory parameter ​*/
    Tracker.reset();
    testFunction(1, () => {
        Tracker.lambdaCalled = true;
    }) 
    {
        Tracker.blockExecuted = true;
    }
    arktest.assertTrue(Tracker.lambdaCalled, "Scenario 3: Lambda should be executed");
    arktest.assertTrue(Tracker.blockExecuted, "Scenario 3: Block should be executed");

    /* Scenario 4: No lambda + block ​*/
    Tracker.reset();
    new TestClass().withOptionalLambda(1) // No lambda parameter
    {
        Tracker.blockExecuted = true;
    }
    arktest.assertTrue(Tracker.blockExecuted, "Boundary Test: Block should be executed");
    arktest.assertEQ(Tracker.lambdaCalled, false)

    /* Scenario 5: only trailing lambda ​*/
    Tracker.reset();
    new TestClass().withOptionalLambda() // No lambda parameter
    {
        Tracker.lambdaCalled = true;
    }
    arktest.assertTrue(Tracker.lambdaCalled, "Boundary Test: Lambda should be executed");
    arktest.assertEQ(Tracker.blockExecuted, false)
}
