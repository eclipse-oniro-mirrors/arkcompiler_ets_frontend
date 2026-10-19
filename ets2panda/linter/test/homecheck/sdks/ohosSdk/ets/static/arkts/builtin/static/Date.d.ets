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

class Date {

    constructor();

    constructor(value: long | string | Date);

    constructor(year: int, monthIndex: int, day?: int, hours?: int, minutes?: int, seconds?: int, ms?: int);

    static now(): long;

    static parse(dateStr: string): long;

    public valueOf(): long;

    public getDate(): int;

    public setDate(value: int): long;

    public getUTCDate(): int;

    public setUTCDate(value: int): long;

    public setUTCDay(value: int): long;

    public getDay(): int;

    public getUTCDay(): int;

    public getYear(): int;

    public getUTCFullYear(): int;

    public getFullYear(): int;

    public setUTCFullYear(value: int): long;

    public setUTCFullYear(value: int, month: int): long;

    public setUTCFullYear(value: int, month: int, date: int): long;

    public setYear(value: int): void;

    public setFullYear(value: int): long;

    public setFullYear(value: int, month: int): long;

    public setFullYear(value: int, month: int, date: int): long;

    public getHours(): int;

    public getUTCHours(): int;

    public setHours(value: int): long;

    public setHours(value: int, min: int): long;

    public setHours(value: int, min: int, sec: int): long;

    public setHours(value: int, min: int, sec: int, ms: int): long;

    public setUTCHours(value: int): long;

    public setUTCHours(value: int, min: int): long;

    public setUTCHours(value: int, min: int, sec: int): long;

    public setUTCHours(value: int, min: int, sec: int, ms: int): long;

    public getMilliseconds(): int;

    public static UTC(d: Date): long;

    public static UTC(year: int): long;

    public static UTC(year: int, month: int, day?: int, hours?: int, minutes?: int, seconds?: int, ms?: int): long;

    public getUTCMilliseconds(): int;

    public setMilliseconds(value: int): long;

    public setUTCMilliseconds(value: int): long;

    public getSeconds(): int;

    public getUTCSeconds(): int;

    public setSeconds(value: int): long;

    public setSeconds(value: int, ms: int): long;

    public setUTCSeconds(value: int): long;

    public setUTCSeconds(value: int, ms: int): long;

    public getMinutes(): int;

    public setUTCMinutes(value: int): long;

    public setUTCMinutes(value: int, sec: int): long;

    public setUTCMinutes(value: int, sec: int, ms: int): long;

    public getUTCMinutes(): int;

    public setMinutes(value: int): long;

    public setMinutes(value: int, sec: int): long;

    public setMinutes(value: int, sec: int, ms: int): long;

    public getMonth(): int;

    public getUTCMonth(): int;

    public setMonth(month: int): long;

    public setMonth(month: int, date: int): long;

    public setUTCMonth(month: int): long;

    public setUTCMonth(month: int, date: int): long;

    public getTime(): long;

    public setTime(value: long): long;

    public getTimezoneOffset(): long;

    public setTimezoneOffset(value: int): long;
}
