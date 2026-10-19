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

class StringBuilder {

    constructor();

    constructor(fromChars: FixedArray<char>);

    constructor(fromChars: char[]);

    constructor(s: string);

    static concatStrings(lhs: string, rhs: string): string;

    append(s: string): StringBuilder;

    append(i: boolean): StringBuilder;

    append(i: byte): StringBuilder;

    append(i: char): StringBuilder;

    append(i: short): StringBuilder;

    append(i: int): StringBuilder;

    append(i: long): StringBuilder;

    append(i: float): StringBuilder;

    append(i: double): StringBuilder;

    append(o: Object): StringBuilder;

    toString(): string;

    static toString(i: boolean): string;

    static toString(i: byte): string;

    static toString(i: char): string;

    static toString(i: short): string;

    static toString(i: int): string;

    static toString(i: long): string;

    static toString(f: float): string;

    static toString(d: double): string;
}
