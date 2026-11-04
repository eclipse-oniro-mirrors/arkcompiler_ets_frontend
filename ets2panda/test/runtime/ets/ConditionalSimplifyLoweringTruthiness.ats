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

class A {}

function objectTruthyIndex(): number {
    let a = new A();
    let arr = [10, 20];
    return arr[a ? 1 : ""];
}

function objectNonStrictNullCheckIndex(): number {
    let a = new A();
    let arr = [30, 40];
    return arr[(a == null) ? "" : 1];
}

function objectStrictUndefinedCheckIndex(): number {
    let a = new A();
    let arr = [50, 60];
    return arr[(a === undefined) ? "" : 1];
}

function trueLiteralIndex(): number {
    let arr = [70, 80];
    return arr[true ? 1 : ""];
}

function falseLiteralIndex(): number {
    let arr = [90, 100];
    return arr[false ? "" : 1];
}

function nonEmptyStringLiteralIndex(): number {
    let arr = [110, 120];
    return arr["x" ? 1 : ""];
}

function emptyStringLiteralIndex(): number {
    let arr = [130, 140];
    return arr["" ? "" : 1];
}

function nonZeroNumberLiteralIndex(): number {
    let arr = [150, 160];
    return arr[1 ? 1 : ""];
}

function zeroNumberLiteralIndex(): number {
    let arr = [170, 180];
    return arr[0 ? "" : 1];
}

function nullLiteralIndex(): number {
    let arr = [190, 200];
    return arr[null ? "" : 1];
}

function undefinedLiteralIndex(): number {
    let arr = [210, 220];
    return arr[undefined ? "" : 1];
}

function nullEqualsNullIndex(): number {
    let maybeNull = null;
    let arr = [230, 240];
    return arr[(maybeNull == null) ? 1 : ""];
}

function undefinedEqualsNullIndex(): number {
    let maybeUndefined = undefined;
    let arr = [250, 260];
    return arr[(maybeUndefined == null) ? 1 : ""];
}

function nullStrictNullIndex(): number {
    let maybeNull = null;
    let arr = [270, 280];
    return arr[(maybeNull === null) ? 1 : ""];
}

function undefinedStrictUndefinedIndex(): number {
    let maybeUndefined = undefined;
    let arr = [290, 300];
    return arr[(maybeUndefined === undefined) ? 1 : ""];
}

function undefinedStrictNullIndex(): number {
    let maybeUndefined = undefined;
    let arr = [310, 320];
    return arr[(maybeUndefined === null) ? "" : 1];
}

function nullStrictUndefinedIndex(): number {
    let maybeNull = null;
    let arr = [330, 340];
    return arr[(maybeNull === undefined) ? "" : 1];
}

function undefinedNotEqualNullIndex(): number {
    let maybeUndefined = undefined;
    let arr = [350, 360];
    return arr[(maybeUndefined != null) ? "" : 1];
}

function nestedKnownConditionalIndex(): number {
    let arr = [370, 380];
    return arr[true ? (false ? "" : 1) : ""];
}

function nestedKnownConditionalAssignment(): number {
    let a = new A();
    let maybeUndefined = undefined;
    let value: number = a ? (maybeUndefined == null ? 1 : "") : "";
    return value;
}

function directKnownConditionalAssignment(): number {
    let a = new A();
    let value: number = a ? 1 : "";
    return value;
}

function nullishKnownConditionalAssignment(): number {
    let maybeNull = null;
    let value: number = (maybeNull == null) ? 1 : "";
    return value;
}

function main(): void {
    arktest.assertEQ(objectTruthyIndex(), 20);
    arktest.assertEQ(objectNonStrictNullCheckIndex(), 40);
    arktest.assertEQ(objectStrictUndefinedCheckIndex(), 60);
    arktest.assertEQ(trueLiteralIndex(), 80);
    arktest.assertEQ(falseLiteralIndex(), 100);
    arktest.assertEQ(nonEmptyStringLiteralIndex(), 120);
    arktest.assertEQ(emptyStringLiteralIndex(), 140);
    arktest.assertEQ(nonZeroNumberLiteralIndex(), 160);
    arktest.assertEQ(zeroNumberLiteralIndex(), 180);
    arktest.assertEQ(nullLiteralIndex(), 200);
    arktest.assertEQ(undefinedLiteralIndex(), 220);
    arktest.assertEQ(nullEqualsNullIndex(), 240);
    arktest.assertEQ(undefinedEqualsNullIndex(), 260);
    arktest.assertEQ(nullStrictNullIndex(), 280);
    arktest.assertEQ(undefinedStrictUndefinedIndex(), 300);
    arktest.assertEQ(undefinedStrictNullIndex(), 320);
    arktest.assertEQ(nullStrictUndefinedIndex(), 340);
    arktest.assertEQ(undefinedNotEqualNullIndex(), 360);
    arktest.assertEQ(nestedKnownConditionalIndex(), 380);
    arktest.assertEQ(nestedKnownConditionalAssignment(), 1);
    arktest.assertEQ(directKnownConditionalAssignment(), 1);
    arktest.assertEQ(nullishKnownConditionalAssignment(), 1);
}
