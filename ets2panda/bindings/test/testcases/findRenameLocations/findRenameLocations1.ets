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

export function abc(x: number): void {
}

export function dummy(x: number): void {
}

export class Foo {
    name: string = "unassigned";
    x: number = 1;
    y: number = 2;
    z: number = 3;
    constructor(name: string, x: number, y: number, z: number) {
        this.name = name;
        this.x = x;
        this.y = y;
        this.z = z;
    }
};

export class Oranges {
    name: string = "unassigned";
    x: number = 1;
    y: number = 2;
    z: number = 3;
    constructor(name: string, x: number, y: number, z: number) {
        this.name = name;
        this.x = x;
        this.y = y;
        this.z = z;
    }
};

dummy(0);
dummy(1);
abc(2);
abc(3);
abc(4);
