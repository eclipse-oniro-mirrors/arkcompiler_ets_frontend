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

class Console {

    log(i: boolean): void;

    log(i: byte): void;

    log(i: short): void;

    log(i: char): void;

    log(i: int): void;

    log(i: long): void;

    log(i: float): void;

    log(i: double): void;

    log(i: string): void;

    log(): void;

    log(...vals: Any[]): void;

    debug(...vals: Any[]): void;

    info(...vals: Any[]): void;

    warn(...vals: Any[]): void;

    error(...vals: Any[]): void;

    assert(...vals: Any[]): void;

    count(label?: string): void;

    countReset(label?: string): void;

    dir(obj?: Any): void;

    dirxml(obj: Any): void;

    group(...objs: Any[]): void;

    groupEnd(): void;

    groupCollapsed(...objs: Any[]): void;

    table(...data: Any[]): void;

    time(label?: string): void;

    timeLog(label?: string, ...arguments: Object[]): void;

    timeEnd(label?: string): void;

    trace(...data: Any[]): void;

    traceHybridStack(): void;
}
