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

function main(){ 
    testHex();
    testDec();
    testOct();
    testBin();
}

function testHex() {
    let n = 0x7fffffff;
    arktest.assertEQ(n,2147483647);

    let n2 = 0x80000000;
    arktest.assertEQ(n2,2147483648);

    let n3 = 0xcccccccc;
    arktest.assertEQ(n3,3435973836);

    let n4 = 0x7fffffffffffffff;
    arktest.assertEQ(n4,9223372036854775807);
}

function testDec() {
    let n = 2147483647;
    arktest.assertEQ(n,2147483647);

    let n2 = 2147483648;
    arktest.assertEQ(n2,2147483648);

    let n3 = 9223372036854775807;
    arktest.assertEQ(n3,9223372036854775807);

    let n4 = -2147483647;
    arktest.assertEQ(n4,-2147483647);

    let n5 = -2147483648;
    arktest.assertEQ(n5,-2147483648);

    let n6 = -9223372036854775807;
    arktest.assertEQ(n6,-9223372036854775807);

    let n7 = -9223372036854775808;
    arktest.assertEQ(n7,-9223372036854775808);
}

function testOct() {
    let n =  0o17777777777;
    arktest.assertEQ(n,2147483647);

    let n2 = 0o20000000000;
    arktest.assertEQ(n2,2147483648);

    let n3 = 0o777777777777777777777;
    arktest.assertEQ(n3,9223372036854775807);
}

function testBin() {
    let n = 0b01111111111111111111111111111111;
    arktest.assertEQ(n,2147483647);

    let n2 = 0b10000000000000000000000000000000;
    arktest.assertEQ(n2,2147483648);

    let n3 = 0b0111111111111111111111111111111111111111111111111111111111111111;
    arktest.assertEQ(n3,9223372036854775807);
}
