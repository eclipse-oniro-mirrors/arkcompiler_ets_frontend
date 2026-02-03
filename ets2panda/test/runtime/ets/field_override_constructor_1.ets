/*
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

class Base1 {
    field: int = 1;

    foo() {
        return 0;
    }
}

class Derived1 extends Base1 {
    field: int = 2;

    foo() {
        return 4;
    }

    constructor() {
        super();
        arktest.assertEQ(this.field, 2);
        this.field = 3;
        arktest.assertEQ(this.field, 3);
        this.field = this.foo();
        arktest.assertEQ(this.field, 4);
    }
}

function outerFunction() {
    return "outer";
}

class Base2 {
    field: string = "base";

    foo() {
        return "error";
    }
}

class Derived2 extends Base2 {
    field: string = "derived";

    foo() {
        return "foo";
    }

    constructor() {
        super();
        arktest.assertEQ(this.field, "derived");
        this.field = "bar";
        arktest.assertEQ(this.field, "bar");
        this.field = this.foo();
        arktest.assertEQ(this.field, "foo");
        this.field = outerFunction();
        arktest.assertEQ(this.field, "outer");
    }
}

function main() {
    let a: Derived1 = new Derived1();
    let b: Derived2 = new Derived2();
}
