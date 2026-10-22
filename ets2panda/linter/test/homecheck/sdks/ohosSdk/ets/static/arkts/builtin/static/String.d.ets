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

class  String {

    constructor();

    constructor(data: FixedArray<char>);

    constructor(otherStr: string);

    constructor(object: Any);

    static $_invoke(): string;

    static $_invoke(value: boolean): string;

    static $_invoke(value: byte): string;

    static $_invoke(value: char): string;

    static $_invoke(value: short): string;

    static $_invoke(value: int): string;

    static $_invoke(value: long): string;

    static $_invoke(value: float): string;

    static $_invoke(value: double): string;

    static $_invoke(value: Any): string;

    $_get(index: int): string;

    getLength(): int;

    get length(): int;

    concat(s1: string, s2: string): string;

    concat(s1: string, s2: string, s3: string): string;

    public localeCompare(that: String, locale?: string | string[], options?: Intl.CollatorOptions): int;

    public startsWith(prefix: String, fromIndex?: int): boolean;

    public endsWith(suffix: String, endPosition?: int): boolean;

    public substring(begin: int): String;

    public substring(begin: int, end: int): String;

    public split(separator: String, limit?: int): String[];

    public split(separator: RegExp, limit?: int): String[];

    public split(separator: String | RegExp, limit?: int): String[];

    public at(index: int): string | undefined;

    public charCodeAt(index: int): int;

    public static fromCharCode(code: int): String;

    public static fromCharCode(...codes: int[]): String;

    public includes(searchString: String, position?: int): boolean;

    public padEnd(end: int): String;

    public padEnd(end: int, ch: char): String;

    public padEnd(end: int, fillString?: String): String;

    public padStart(end: int, ch: char): String;

    public padStart(end: int, fillString?: String): String;

    public substr(begin: int, length?: int): String;

    public search(implicitRegExp: String): int;

    public search(regexp: RegExp): int;

    public search(regexp: String | RegExp): int;

    public static fromCodePoint(...codePoints: int[]): String;
}
