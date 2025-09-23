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

interface Interface_1 {}
interface Interface_2 {}

class GenericObject<T extends Interface_1 = Interface_2> {
  give(value: T) {
    return value;
  }
}

interface Factory<T> {
  create(): T;
}

class GenericObjectFactory implements Factory<GenericObject> {
  create(): GenericObject {
    return new GenericObject();
  }
}

function g<T>(factory: Factory<T>): T {
  return factory.create();
}

const factory = new GenericObjectFactory();
const obj = g(factory);
obj.give({});

/* @@? 19:58 Error TypeError: Type argument 'Interface_2' should be a subtype of 'Interface_1' constraint */
