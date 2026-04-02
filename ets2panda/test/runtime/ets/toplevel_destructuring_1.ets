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

let counter = 0;
function incrementCounter(): number {
    counter++;
    return counter;
}

let tupleforDestruct: [number, string, boolean] = [incrementCounter(), "text", true];
let [num, str, bool] = tupleforDestruct;

class TestClass {
    static staticNum: number = num;
    static staticStr: string = str;
    static staticBool: boolean = bool;

    static getValues(): [number, string, boolean] {
        return [TestClass.staticNum, TestClass.staticStr, TestClass.staticBool];
    }
}

function main(): void {
    {
        // Verify: incrementCounter() is called only once when creating the tuple. At this point counter should be 1
        arktest.assertEQ(counter, 1);

        // Verify: num should be the return value of the first incrementCounter() call: 1
        arktest.assertEQ(num, 1);
        arktest.assertEQ(str, "text");
        arktest.assertEQ(bool, true);
    }

    {
        let currentCounter = counter;
        let _ = TestClass.staticNum;
        let __ = TestClass.staticStr;
        let ___ = TestClass.staticBool;

        // Verify: counter did not increase - cloning does not execute the function
        arktest.assertEQ(counter, currentCounter);
        arktest.assertEQ(TestClass.staticNum, 1);
        arktest.assertEQ(TestClass.staticStr, "text");
        arktest.assertEQ(TestClass.staticBool, true);
    }

    {
        let values = TestClass.getValues();
        arktest.assertEQ(values[0], 1);
        arktest.assertEQ(values[1], "text");
        arktest.assertEQ(values[2], true);

        // Verify: counter should still not have increased
        arktest.assertEQ(counter, 1);
    }
}
