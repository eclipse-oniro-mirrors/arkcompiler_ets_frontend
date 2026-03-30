/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function defaultValue(defaultVal: any) {
    return function(target: any, propertyKey: string) {
        let value = defaultVal;

        Object.defineProperty(target, propertyKey, {
            get: () => value,
            set: (newVal) => { value = newVal; },
            enumerable: true,
            configurable: true
        });
    };
}

class Person {
    @defaultValue('name')
    name: string;

    @defaultValue(18)
    age: number;
}

const p1 = new Person();
print(p1.name, p1.age);

const p2 = new Person();
p2.name = 'Jone';
p2.age = 25;
