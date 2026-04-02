/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

abstract class AbstractClass {
    private _age: int;
    private _value: int;

    abstract get age(): int;
    abstract set age(a: int);
    abstract set value(v: int);
    abstract get value(): int;
}

class ChildClass extends AbstractClass {
    private _age: int = 0;
    private _value: int = 0;


    override get age(): int {
        return this._age;
    }

    override set age(a: int) {
        this._age = a;
    }

    override set value(v: int) {
        this._value = v;
    }

    override get value(): int {
        return this._value;
    }
}
