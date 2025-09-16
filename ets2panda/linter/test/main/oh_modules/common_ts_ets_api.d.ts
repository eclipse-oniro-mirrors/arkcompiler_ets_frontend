/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

export declare interface IMonitor {
  value<T>(path?: string): IMonitorValue<T> | undefined;
}
export declare interface IMonitorValue<T> {
    before: T;
    now: T;
    path: string;
}
export declare class LocalStorage {
  get<T>(propName: string): T | undefined;
  ref<T>(propName: string): AbstractProperty<T> | undefined;
  link<T>(propName: string): SubscribedAbstractProperty<T> | undefined;
}

export declare class AppStorage {
  static get<T>(propName: string): T | undefined;
  static ref<T>(propName: string): AbstractProperty<T> | undefined;
  static link<T>(propName: string): SubscribedAbstractProperty<T> | undefined;
}

export declare interface AbstractProperty<T> {
  get(): T;
  set(newValue: T): void;
  info(): string;
}
export declare abstract class SubscribedAbstractProperty<T> {
  info(): string;
  abstract get(): T;
  abstract set(newValue: T): void;
  abstract aboutToBeDeleted(): void;
}