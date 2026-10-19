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

declare class AppStorage {

    static ref<T>(propName: string): AbstractProperty<T> | undefined;

    static setAndRef<T>(propName: string, defaultValue: T): AbstractProperty<T>;

    static Link(propName: string): any;

    static link<T>(propName: string): SubscribedAbstractProperty<T>;

    static SetAndLink<T>(propName: string, defaultValue: T): SubscribedAbstractProperty<T>;

    static setAndLink<T>(propName: string, defaultValue: T): SubscribedAbstractProperty<T>;

    static Prop(propName: string): any;

    static prop<T>(propName: string): SubscribedAbstractProperty<T>;

    static SetAndProp<S>(propName: string, defaultValue: S): SubscribedAbstractProperty<S>;

    static setAndProp<T>(propName: string, defaultValue: T): SubscribedAbstractProperty<T>;

    static Has(propName: string): boolean;

    static has(propName: string): boolean;

    static Get<T>(propName: string): T | undefined;

    static get<T>(propName: string): T | undefined;

    static Set<T>(propName: string, newValue: T): boolean;

    static set<T>(propName: string, newValue: T): boolean;

    static SetOrCreate<T>(propName: string, newValue: T): void;

    static setOrCreate<T>(propName: string, newValue: T): void;

    static Delete(propName: string): boolean;

    static delete(propName: string): boolean;

    static Keys(): IterableIterator<string>;

    static keys(): IterableIterator<string>;

    static staticClear(): boolean;

    static Clear(): boolean;

    static clear(): boolean;

    static IsMutable(propName: string): boolean;

    static Size(): number;

    static size(): number;
}

declare interface AbstractProperty<T> {

    get(): T;

    set(newValue: T): void;

    info(): string;
}

declare abstract class SubscribedAbstractProperty<T> {

    info(): string;

    abstract get(): T;

    abstract set(newValue: T): void;

    abstract aboutToBeDeleted(): void;
}

declare interface EnvPropsOptions {

    key: string;

    defaultValue: number | string | boolean;
}

declare class Environment {

    static EnvProp<S>(key: string, value: S): boolean;

    static envProp<S>(key: string, value: S): boolean;

    static EnvProps(props: {
        key: string;
        defaultValue: any;
    }[]): void;

    static envProps(props: EnvPropsOptions[]): void;

    static Keys(): Array<string>;

    static keys(): Array<string>;
}

declare interface PersistPropsOptions {

    key: string;

    defaultValue: number | string | boolean | Object;
}

declare class PersistentStorage {

    static PersistProp<T>(key: string, defaultValue: T): void;

    static persistProp<T>(key: string, defaultValue: T): void;

    static DeleteProp(key: string): void;

    static deleteProp(key: string): void;

    static PersistProps(properties: {
        key: string;
        defaultValue: any;
    }[]): void;

    static persistProps(props: PersistPropsOptions[]): void;

    static Keys(): Array<string>;

    static keys(): Array<string>;
}

declare class LocalStorage {

    constructor(initializingProperties?: Object);

    static GetShared(): LocalStorage;

    static getShared(): LocalStorage;

    public ref<T>(propName: string): AbstractProperty<T> | undefined;

    public setAndRef<T>(propName: string, defaultValue: T): AbstractProperty<T>;

    has(propName: string): boolean;

    keys(): IterableIterator<string>;

    size(): number;

    get<T>(propName: string): T | undefined;

    set<T>(propName: string, newValue: T): boolean;

    setOrCreate<T>(propName: string, newValue: T): boolean;

    link<T>(propName: string): SubscribedAbstractProperty<T>;

    setAndLink<T>(propName: string, defaultValue: T): SubscribedAbstractProperty<T>;

    prop<S>(propName: string): SubscribedAbstractProperty<S>;

    setAndProp<S>(propName: string, defaultValue: S): SubscribedAbstractProperty<S>;

    delete(propName: string): boolean;

    clear(): boolean;
}
