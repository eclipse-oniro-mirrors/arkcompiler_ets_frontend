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
    function setBatteryConfig(sceneName: string, sceneValue: string): number;
}

declare namespace systemDateTime {
    enum TimeType {
        STARTUP = 0,
        ACTIVE = 1
    }
    function getTime(isNanoseconds?: boolean): number;
    function getUptime(timeType: TimeType, isNanoseconds?: boolean): number;
}

declare namespace image {
    interface Size {
        width: number;
        height: number;
    }

    interface Image {
        readonly timestamp: number;
    }
}

declare namespace outerNS {
    namespace innerNS {
        const MAX_LENGTH: number;
    }

    namespace innerNS1 {
        namespace innerNS2 {
            const MIN: number;
        }
    }
}

declare namespace rpc {
    class IPCSkeleton {
        static getCallingPid(): number;
        static getCallingTokenId(): number;
    }

    class AtomicServiceOptions {
        flags?: number;
    }
}

declare function parseInt(str: string, radix?: number): number;
declare function parseInt(str: string): number;

declare function parseLong(num: number): number;

declare function parseIntOrLong(num: number): number;

declare function getArg(): number;

declare function parseFloat(str: string): number;

declare namespace testAsyncNS {
    interface TestAsync {
        getArgAsyncNumber(): Promise<number>;

        getArgAsyncInt(): Promise<number>;
    }
}

export { parseInt, parseFloat };