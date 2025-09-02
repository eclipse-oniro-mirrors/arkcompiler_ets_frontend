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

export class Foo{}
export let foo = new Foo()
export function CreatePerson(name, age) {
    return {
      name,
      age,
      greet() {
        return `Hello, I'm ${this.name}`;
      }
    };
}
export const a = [1,2,3]
export function b () {
    return [1,2,3]
}
export class Animal {}
export class Dog extends Animal {}
export const MyNamespace = {
    Animal: class {
      constructor(name) {
        this.name = name;
      }
    },
    Dog: class extends MyNamespace.Animal {
      bark() {
        return 'Woof!';
      }
    }
};