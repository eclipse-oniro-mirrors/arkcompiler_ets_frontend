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

declare namespace Session{
  interface AV{}
  type A = 'play' | 'pause';
}

export class A {
  id?: string;
  map?: Map<string, B>;
}

export class B {}
export class C11 {}

export default Session;

export type EventListener<T> = (data: T) => void;

export namespace Event{
  export type EventEmitter<T> = (data: T) => void;
}

export class EventBus<T> {
  private eventListeners: Map<string, EventListener<T>> = new Map();
}
