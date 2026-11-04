/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class A  {
    a: int = 1;
    b: int = 3;
    public constructor() {
        this.a = 2;
        this.b = 4;
        this.a = 10;
        this.b = 20;
    }

    public constructor(p : int) {
        this();
        this.a = 30;
        this.b = 40;
    }

    public constructor(s : String) {
        this.a = 2;
        this.b = 4;
        this.a = 50;
        this.b = 60;
    }

}

class B extends A  {
    c: int = 11;
    d: int = 33;
    public constructor() {
        this.c += 22;
        this.d -= 44;
        this.c = 100;
        this.d = 200;
    }

    public constructor(p : int) {
        this();
        this.c = 300;
        this.d = 400;
    }

    public constructor(s : String) {
        super(0);
        this.c += 22;
        this.d -= 44;
        this.c = 500;
        this.d = 600;
    }

}
