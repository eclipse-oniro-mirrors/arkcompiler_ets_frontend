'use static';
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class Math {

    static ceil(x: double): double;

    static floor(x: double): double;

    static round(x: double): double;

    static sqrt(x: double): double;

    static min(): double;

    static min(val: double): double;

    static min(val1: double, val2: double): double;

    static min(...values: double[]): double;

    static max(): double;

    static max(val: double): double;

    static max(val1: double, val2: double): double;

    static max(...values: double[]): double;

    static abs(x: byte): byte;

    static abs(x: short): short;

    static abs(x: int): int;

    static abs(x: long): long;

    static abs(x: float): float;

    static abs(x: double): double;

    static log2(x: double): double;

    static log2(x: float): float;

    static pow(u: double, v: double): double;

    static random(): double;
}
