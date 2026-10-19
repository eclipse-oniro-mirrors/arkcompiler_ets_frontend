/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
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

declare namespace batteryInfo {
    function setBatteryConfig(sceneName: string, sceneValue: string): int;
}

declare namespace systemDateTime {
    enum TimeType {
        STARTUP = 0,
        ACTIVE = 1
    }
    function getTime(isNanoseconds?: boolean): long;
    function getUptime(timeType: TimeType, isNanoseconds?: boolean): long;
}

declare namespace image {
    interface Size {
        width: int;
        height: int;
    }

    interface Image {
        readonly timestamp: long;
    }
}

declare namespace outerNS {
    namespace innerNS {
        const MAX_LENGTH: long;
    }

    namespace innerNS1 {
        namespace innerNS2 {
            const MIN: int;
        }
    }
}

declare namespace rpc {
    class IPCSkeleton {
        static getCallingPid(): int;
        static getCallingTokenId(): long;
    }

    class AtomicServiceOptions {
        flags?: int;
    }
}

declare function parseInt(str: string, radix?: int): int;
declare function parseInt(str: string): int;

declare function parseLong(num: long): long;

declare function parseIntOrLong(num: int): int;
declare function parseIntOrLong(num: long): long;

declare function getArg(): int;

declare namespace testAsyncNS {
    interface TestAsync {
        getArgAsyncNumber(): Promise<number>;

        getArgAsyncInt(): Promise<int>;
    }
}

declare function parseFloat(str: string): number;

export { parseInt, parseFloat };