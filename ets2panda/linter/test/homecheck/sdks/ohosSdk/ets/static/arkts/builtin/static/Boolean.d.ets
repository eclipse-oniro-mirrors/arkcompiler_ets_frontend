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

class Boolean extends Object implements Comparable<boolean> {

    constructor(value: boolean);

    constructor(value: byte);

    constructor(value: char);

    constructor(value: short);

    constructor(value: int);

    constructor(value: long);

    constructor(value: bigint);

    constructor(value: float);

    constructor(value: string);

    constructor(value: Any = undefined);

    static $_invoke(): boolean;

    static $_invoke<T>(value: T): boolean;

    static toBoolean(value: boolean): boolean;

    toBoolean(): boolean;
}
