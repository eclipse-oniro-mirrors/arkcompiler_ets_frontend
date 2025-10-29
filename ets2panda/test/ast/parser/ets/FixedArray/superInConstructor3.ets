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

class A {
    private _x: int;

    get x(): int{
        return this._x;
    }

    public constructor() {
        this._x = 1;
    }

    public constructor(arg: boolean) {
    }
}

class alma extends A {
    public constructor() {
        this([(((typeof ((new A(((new A(foo(foo({x: /* @@ label */super.x})).x == 5)).x + 5 as int) == 3)) instanceof A ? 1 : 2)) == "number" ? 10 : null)!) ?? 10]);
    }
    public constructor(arg : A) {
    }
    public constructor(arg : int) {
    }
    public constructor(arg : FixedArray<Int>) {
    }
}

function foo(arg : A) {
    return arg;
}


/* @@@ label Error Semantic error ESE0235: Using super is not allowed in constructor */