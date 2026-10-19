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

export class JSON {

    stringify(d: byte): string;

    stringify(d: char): string;

    stringify(d: short): string;

    stringify(d: int): string;

    stringify(d: long): string;

    stringify(d: float): string;

    stringify(d: double): string;

    stringify(d: bigint): string;

    stringify(d: boolean): string;

    stringify(d: string): string;

    stringify(d: FixedArray<byte>): string;

    stringify(d: FixedArray<char>): string;

    stringify(d: FixedArray<short>): string;

    stringify(d: FixedArray<int>): string;

    stringify(d: FixedArray<long>): string;

    stringify(d: FixedArray<float>): string;

    stringify(d: FixedArray<double>): string;

    stringify(d: FixedArray<boolean>): string;

    stringify(d: Array<Double>): string;

    stringify(d: ArrayLike<Double>): string;

    stringify(obj: JsonReplacer): string;

    stringify(obj: Any): string;

    stringify(obj: Any, replacer: ((key: string, value: Any) => Any) | undefined | null, space?: string | int): string;

    stringify(obj: Any, replacer: FixedArray<double | string>, space?: int | string): string;

    stringify(obj: Any, replacer: Array<double | string>, space?: int | string): string;

    parse<T>(text: string, type: Type): T | null | undefined;

    parse<T>(text: string, reviver: ((key: string, value: Any) => Any) | undefined, type: Type, options?: jsonx.ParseOptions): T | null | undefined;

    parse<T>(json: string, type: Class): T | null | undefined;

    parse<T>(json: string, reviver: ((key: string, value: Any) => Any) | undefined, type: Class, options?: jsonx.ParseOptions): T | null | undefined;

    parseJsonElement(text: string, options?: jsonx.ParseOptions): jsonx.JsonElement;

    parseJsonElement(text: string, reviver: (key: string, value: jsonx.JsonElement) => jsonx.JsonElement, options?: jsonx.ParseOptions): jsonx.JsonElement;

    stringifyWithGetters(obj: Any): string;

    stringifyWithGetters(
      obj: Any,
      replacer: ((key: string, value: Any) => Any) | undefined | null,
      space?: int | string): string;

    stringifyWithGetters(
      obj: Any,
      replacer: Array<double | string> | Array<string> | Array<double>,
      space?: int | string): string;

    parseUpdate<T>(json: string, instance: T): T;

    parseUpdate<T>(
        json: string,
        reviver: ((key: string, value: Any) => Any) | undefined,
        instance: T,
        options?: jsonx.ParseOptions
    ): T;
}
