/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Test function calls in array access with compound assignment
let callCount = 0;
let result: string = "";
let testArray: int[] = [1, 2, 3];

function getArray(): int[] {
    callCount++;
    result = result + "getArray ";
    return testArray;
}

function getIndex(ind: int): int {
    callCount++;
    result = result + "getIndex ";
    return ind;
}

function getValue(val: int): int {
    callCount++;
    result = result + "getValue ";
    return val;
}

class Container {
    data: int[] = [5, 10, 15];
    
    getData(): int[] {
        callCount++;
        result = result + "getData ";
        return this.data;
    }
    
    getIndex(idx: int): int {
        callCount++;
        result = result + "getIndex ";
        return idx;
    }
}

class Matrix {
    rows: int[][] = [[1, 2], [3, 4]];
    
    getRow(i: int): int[] {
        callCount++;
        result = result + "getRow ";
        return this.rows[i];
    }
}

function testFunctionCallInArrayAccess() {
    callCount = 0;
    result = "";
    testArray = [1, 2, 3];
    (getArray())[getIndex(1)] *= getValue(2);
    arktest.assertEQ(callCount, 3); // Each function should be called exactly once
    arktest.assertEQ(result, "getArray getIndex getValue ");
    arktest.assertEQ(testArray[1], 4); // 2 * 2 = 4
}

function testNestedMemberWithFunctionCalls() {
    let container = new Container();
    callCount = 0;
    result = "";
    container.getData()[container.getIndex(1)] += getValue(5);
    arktest.assertEQ(callCount, 3); // getData, getIndex, getValue each called once
    arktest.assertEQ(result, "getData getIndex getValue ");
    arktest.assertEQ(container.data[1], 15); // 10 + 5 = 15
}

function testMemberExpressionChain() {
    let matrix = new Matrix();
    callCount = 0;
    result = "";
    matrix.getRow(0)[getIndex(1)] += getValue(10);
    arktest.assertEQ(callCount, 3); // getRow, getIndex, getValue each called once
    arktest.assertEQ(result, "getRow getIndex getValue ");
    arktest.assertEQ(matrix.rows[0][1], 12); // 2 + 10 = 12
}

function main() {
    testFunctionCallInArrayAccess();
    testNestedMemberWithFunctionCalls();
    testMemberExpressionChain();
}

