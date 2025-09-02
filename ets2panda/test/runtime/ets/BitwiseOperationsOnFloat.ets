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

function And(a: double, b: double): long {
    return a & b
}

function Or(a: double, b: double): long {
    return a | b
}

function Xor(a: double, b: double): long {
    return a ^ b
}

function LeftShift(a: double, b: double): long {
    return a << b
}

function RightShift(a: double, b: double): long {
    return a >> b
}

function UnsignedRightShift(a: double, b: double): long {
    return a >>> b
}
 
function Inversion(a: double): long {
    return ~a;
}

function main(): void {
    /* ---- Compile time tests ---- */

    // Testing "and" operator:
    arktest.assertEQ((542.910991 & -1903.2040221), 16)
    arktest.assertEQ((-542.910991 & 1903.2040221), 1378)
    arktest.assertEQ((-542.910991 & -1903.2040221), -1920)
    arktest.assertEQ((9E120 & 56.75), 56)
    arktest.assertEQ((0.0 & 0.0), 0)
    arktest.assertEQ((NaN & 42.131330352), 0)
    arktest.assertEQ((-NaN & -23432.34110144432), 0)
    arktest.assertEQ((Infinity & -94.24445985981884), 9223372036854775714)
    arktest.assertEQ((-Infinity & 94.24445985981884), 0)

    // Testing "or" operator: 
    arktest.assertEQ((542.910991 | 1903.2040221), 1919)
    arktest.assertEQ((542.910991 | -1903.2040221), -1377)
    arktest.assertEQ((-542.910991 | 1903.2040221), -17)
    arktest.assertEQ((-542.910991 | -1903.2040221), -525)
    arktest.assertEQ((9E120 | 0), 9223372036854775807)
    arktest.assertEQ((824E3 | 21.018763), 824021)
    arktest.assertEQ((1.0 | 0.1), 1)
    arktest.assertEQ((NaN | 0.2), 0)
    arktest.assertEQ((-NaN | 0.3), 0)
    arktest.assertEQ((Infinity | 0.4), 9223372036854775807)
    arktest.assertEQ((-Infinity | 0.5), -9223372036854775808)

    // Testing "xor" operator: 
    arktest.assertEQ((542.910991 ^ 1903.2040221), 1393)
    arktest.assertEQ((542.910991 ^ -1903.2040221), -1393)
    arktest.assertEQ((-542.910991 ^ 1903.2040221), -1395)
    arktest.assertEQ((-542.910991 ^ -1903.2040221), 1395)
    arktest.assertEQ((49509.2348100001 ^ 49509.2348100001), 0)
    arktest.assertEQ((9E120 ^ 1.2), 9223372036854775806)
    arktest.assertEQ((824E3 ^ 21.018763), 824021)
    arktest.assertEQ((NaN ^ 99854258.24), 99854258)
    arktest.assertEQ((-NaN ^ 30483040.293244), 30483040)
    arktest.assertEQ((Infinity ^ 1.2), 9223372036854775806)
    arktest.assertEQ((-Infinity ^ 10049329.80001), -9223372036844726479)

    // Testing "left shift" operator:
    arktest.assertEQ((1E-100 << 0.0), 0)
    arktest.assertEQ((0.00003 << 12.13), 0)
    arktest.assertEQ((42.109 << 0.0120939), 42)
    arktest.assertEQ((1.409240940 << 17.3), 131072)
    arktest.assertEQ((4342435.309421 << 3.1), 34739480)
    arktest.assertEQ((9010034745.3449093132 << 7.000000000001), 1153284447360)
    arktest.assertEQ((-423.14981 << 20.32), -443547648)
    arktest.assertEQ((500.13 << 128.3440), 500)
    arktest.assertEQ((500.45 << 121.10000002), -1729382256910270464)
    arktest.assertEQ((NaN << 330.3911), 0)
    arktest.assertEQ((-NaN << 12.91213), 0)
    arktest.assertEQ((Infinity << 1.0092), -2)
    arktest.assertEQ((-Infinity << 1.0092), 0)
    arktest.assertEQ((-52242.2301 << -8.7), -1297036692682702848)
    arktest.assertEQ((52242.2301 << -8.7), 1297036692682702848)

    // Testing "right shift" operator:
    arktest.assertEQ((1E-100 >> 0.0), 0)
    arktest.assertEQ((0.00003 >> 12.13), 0)
    arktest.assertEQ((42.109 >> 0.0120939), 42)
    arktest.assertEQ((1.409240940 >> 17.3), 0)
    arktest.assertEQ((4342435.309421 >> 3.1), 542804)
    arktest.assertEQ((9010034.3449093132 >> 3.000000000001), 1126254)
    arktest.assertEQ((-4599090490.24 >> 11), -2245650)
    arktest.assertEQ((500.13 >> 128.3440), 500)
    arktest.assertEQ((500.45 >> 121.10000002), 0)
    arktest.assertEQ((NaN >> 11.000003), 0)
    arktest.assertEQ((-NaN >> 7.912130001), 0)
    arktest.assertEQ((Infinity >> 61), 3)
    arktest.assertEQ((-Infinity >> 61), -4)
    arktest.assertEQ((132090941883.34343 >> -32.2), 30)
    arktest.assertEQ((-132090941883.34343 >> -32.2), -31)

    // Testing "unsigned right shift" operator:
    arktest.assertEQ((1E-100 >>> 0.0), 0)
    arktest.assertEQ((0.00003 >>> 12.13), 0)
    arktest.assertEQ((42.109 >>> 0.0120939), 42)
    arktest.assertEQ((1.409240940 >>> 17.3), 0)
    arktest.assertEQ((4342435.309421 >>> 3.1), 542804)
    arktest.assertEQ((9010034.3449093132 >>> 3.000000000001), 1126254)
    arktest.assertEQ((-4599090490.24 >>> 11), 9007199252495342)
    arktest.assertEQ((500.13 >>> 128.3440), 500)
    arktest.assertEQ((500.45 >>> 121.10000002), 0)
    arktest.assertEQ((NaN >>> 11.000003), 0)
    arktest.assertEQ((-NaN >>> 7.912130001), 0)
    arktest.assertEQ((Infinity >>> 61.8), 3)
    arktest.assertEQ((-Infinity >>> 61.8), 4)
    arktest.assertEQ((132090941883.34343 >>> -32.2), 30)
    arktest.assertEQ((-132090941883.34343 >>> -32.2), 4294967265)

    // Testing "bitwise complement" operator
    arktest.assertEQ(~0, -1)
    arktest.assertEQ(~0.000034, -1)
    arktest.assertEQ(~39530.93, -39531)
    arktest.assertEQ(~93718001.5424230894, -93718002)
    arktest.assertEQ(~Infinity, -9223372036854775808)
    arktest.assertEQ(~-Infinity, 9223372036854775807)
    arktest.assertEQ(~NaN, -1   )
    arktest.assertEQ(~-NaN, -1)
    arktest.assertEQ(~1E210, -9223372036854775808)
    arktest.assertEQ(~-1E210,  9223372036854775807)
    arktest.assertEQ(~56E5, -5600001)
    arktest.assertEQ(~-56E5, 5599999)

    /* ---- Run time tests ---- */

    // Testing "and" operator:
    arktest.assertEQ(And(542.910991, -1903.2040221), 16)
    arktest.assertEQ(And(-542.910991, 1903.2040221), 1378)
    arktest.assertEQ(And(-542.910991, -1903.2040221), -1920)
    arktest.assertEQ(And(9E120, 56.75), 56)
    arktest.assertEQ(And(0.0, 0.0), 0)
    arktest.assertEQ(And(NaN, 42.131330352), 0)
    arktest.assertEQ(And(-NaN, -23432.34110144432), 0)
    arktest.assertEQ(And(Infinity, -94.24445985981884), 9223372036854775714)
    arktest.assertEQ(And(-Infinity, 94.24445985981884), 0)

    // Testing "or" operator: 
    arktest.assertEQ(Or(542.910991, 1903.2040221), 1919)
    arktest.assertEQ(Or(542.910991, -1903.2040221), -1377)
    arktest.assertEQ(Or(-542.910991, 1903.2040221), -17)
    arktest.assertEQ(Or(-542.910991, -1903.2040221), -525)
    arktest.assertEQ(Or(9E120, 0), 9223372036854775807)
    arktest.assertEQ(Or(824E3, 21.018763), 824021)
    arktest.assertEQ(Or(1.0, 0.1), 1)
    arktest.assertEQ(Or(NaN, 0.2), 0)
    arktest.assertEQ(Or(-NaN, 0.3), 0)
    arktest.assertEQ(Or(Infinity, 0.4), 9223372036854775807)
    arktest.assertEQ(Or(-Infinity, 0.5), -9223372036854775808)

    // Testing "xor" operator: 
    arktest.assertEQ(Xor(542.910991, 1903.2040221), 1393)
    arktest.assertEQ(Xor(542.910991, -1903.2040221), -1393)
    arktest.assertEQ(Xor(-542.910991, 1903.2040221), -1395)
    arktest.assertEQ(Xor(-542.910991, -1903.2040221), 1395)
    arktest.assertEQ(Xor(49509.2348100001, 49509.2348100001), 0)
    arktest.assertEQ(Xor(9E120, 1.2), 9223372036854775806)
    arktest.assertEQ(Xor(824E3, 21.018763), 824021)
    arktest.assertEQ(Xor(NaN, 99854258.24), 99854258)
    arktest.assertEQ(Xor(-NaN, 30483040.293244), 30483040)
    arktest.assertEQ(Xor(Infinity, 1.2), 9223372036854775806)
    arktest.assertEQ(Xor(-Infinity, 10049329.80001), -9223372036844726479)

    // Testing "left shift" operator:
    arktest.assertEQ(LeftShift(1E-100, 0.0), 0)
    arktest.assertEQ(LeftShift(0.00003, 12.13), 0)
    arktest.assertEQ(LeftShift(42.109, 0.0120939), 42)
    arktest.assertEQ(LeftShift(1.409240940, 17.3), 131072)
    arktest.assertEQ(LeftShift(4342435.309421, 3.1), 34739480)
    arktest.assertEQ(LeftShift(9010034745.3449093132, 7.000000000001), 1153284447360)
    arktest.assertEQ(LeftShift(-423.14981, 20.32), -443547648)
    arktest.assertEQ(LeftShift(500.13, 128.3440), 500)
    arktest.assertEQ(LeftShift(500.45, 121.10000002), -1729382256910270464)
    arktest.assertEQ(LeftShift(NaN, 330.3911), 0)
    arktest.assertEQ(LeftShift(-NaN, 12.91213), 0)
    arktest.assertEQ(LeftShift(Infinity, 1.0092), -2)
    arktest.assertEQ(LeftShift(-Infinity, 1.0092), 0)
    arktest.assertEQ(LeftShift(-52242.2301, -8.7), -1297036692682702848)
    arktest.assertEQ(LeftShift(52242.2301,-8.7), 1297036692682702848)

    // Testing "right shift" operator:
    arktest.assertEQ(RightShift(1E-100, 0.0), 0)
    arktest.assertEQ(RightShift(0.00003, 12.13), 0)
    arktest.assertEQ(RightShift(42.109, 0.0120939), 42)
    arktest.assertEQ(RightShift(1.409240940, 17.3), 0)
    arktest.assertEQ(RightShift(4342435.309421, 3.1), 542804)
    arktest.assertEQ(RightShift(9010034.3449093132, 3.000000000001), 1126254)
    arktest.assertEQ(RightShift(-4599090490.24, 11), -2245650)
    arktest.assertEQ(RightShift(500.13, 128.3440), 500)
    arktest.assertEQ(RightShift(500.45, 121.10000002), 0)
    arktest.assertEQ(RightShift(NaN, 11.000003), 0)
    arktest.assertEQ(RightShift(-NaN, 7.912130001), 0)
    arktest.assertEQ(RightShift(Infinity, 61), 3)
    arktest.assertEQ(RightShift(-Infinity, 61), -4)
    arktest.assertEQ(RightShift(132090941883.34343, -32.2), 30)
    arktest.assertEQ(RightShift(-132090941883.34343, -32.2), -31)

    // Testing "unsigned right shift" operator:
    arktest.assertEQ(UnsignedRightShift(1E-100, 0.0), 0)
    arktest.assertEQ(UnsignedRightShift(0.00003,12.13), 0)
    arktest.assertEQ(UnsignedRightShift(42.109, 0.0120939), 42)
    arktest.assertEQ(UnsignedRightShift(1.409240940, 17.3), 0)
    arktest.assertEQ(UnsignedRightShift(4342435.309421, 3.1), 542804)
    arktest.assertEQ(UnsignedRightShift(9010034.3449093132, 3.000000000001), 1126254)
    arktest.assertEQ(UnsignedRightShift(-4599090490.24, 11), 9007199252495342)
    arktest.assertEQ(UnsignedRightShift(500.13, 128.3440), 500)
    arktest.assertEQ(UnsignedRightShift(500.45, 121.10000002), 0)
    arktest.assertEQ(UnsignedRightShift(NaN, 11.000003), 0)
    arktest.assertEQ(UnsignedRightShift(-NaN, 7.912130001), 0)
    arktest.assertEQ(UnsignedRightShift(Infinity, 61.8), 3)
    arktest.assertEQ(UnsignedRightShift(-Infinity, 61.8), 4)
    arktest.assertEQ(UnsignedRightShift(132090941883.34343, -32.2), 30)
    arktest.assertEQ(UnsignedRightShift(-132090941883.34343, -32.2), 4294967265)
 
    // Testing "bitwise complement" operator
    arktest.assertEQ(Inversion(0), -1)
    arktest.assertEQ(Inversion(0.000034), -1)
    arktest.assertEQ(Inversion(39530.93), -39531)
    arktest.assertEQ(Inversion(93718001.5424230894), -93718002)
    arktest.assertEQ(Inversion(Infinity), -9223372036854775808)
    arktest.assertEQ(Inversion(-Infinity), 9223372036854775807)
    arktest.assertEQ(Inversion(NaN), -1   )
    arktest.assertEQ(Inversion(-NaN), -1)
    arktest.assertEQ(Inversion(1E210), -9223372036854775808)
    arktest.assertEQ(Inversion(-1E210),  9223372036854775807)
    arktest.assertEQ(Inversion(56E5), -5600001)
    arktest.assertEQ(Inversion(-56E5), 5599999)
}
