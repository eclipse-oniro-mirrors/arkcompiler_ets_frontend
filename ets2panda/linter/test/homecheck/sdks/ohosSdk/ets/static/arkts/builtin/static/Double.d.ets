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

class Double extends Floating implements Comparable<Double> {

    constructor();

    constructor(value: double);

    constructor(value: BigInt);

    constructor(value: string);

    static $_invoke(): Double;

    static $_invoke(value: string | Double | BigInt | undefined | null): Double;

    toDouble(): double;

    static toDouble(value: double): double;

    static parseFloat(s: string): double;

    static parseInt(s: string): double;

  static parseInt(s: string, r: int): double;

    static parseInt(s: string, r: double): double;

    equals(other: Any): boolean;

    static isNaN(v: double): boolean;

    isNaN(): boolean;

    static isFinite(v: double): boolean;

    isFinite(): boolean;
}
