/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export declare class LocalStorage {
    constructor(initializingProperties?: Object);
    static getShared(): LocalStorage;
}

export declare class Environment {
    static EnvProp<S>(key: string, value: S): boolean;
    static Keys(): Array<string>;
    static EnvProps(props: {
        key: string;
        defaultValue: any;
    }[]): void;
}

export declare class AppStorage {
    static IsMutable(propName: string): boolean;
    static Get<T>(propName: string): T | undefined;
    static Clear(): boolean;
    static Delete(propName: string): boolean;
    static Size(): number;
}

export declare class PersistentStorage {
    static PersistProps(properties: {
        key: string;
        defaultValue: any;
    }[]): void;
    static Keys(): Array<string>;
}