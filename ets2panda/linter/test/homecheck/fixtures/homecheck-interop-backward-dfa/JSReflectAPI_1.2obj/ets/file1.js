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

export function goo(obj) {
  foo(obj)
}

export function foo(obj) {
  Reflect.apply(obj.getName, {a : 12});
  Reflect.defineProperty(obj, 'newField', { value: 7 });
  Reflect.deleteProperty(obj, "a");
  Reflect.getOwnPropertyDescriptor(obj, "a");
  Reflect.ownKeys(obj);
  Reflect.isExtensible(obj);
  Reflect.get(obj, 'a');
  Reflect.set(obj, 'a', 'world');
  Reflect.setPrototypeOf(obj, {});
}
