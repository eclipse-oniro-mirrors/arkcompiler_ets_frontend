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

class Base {
    private fooPrivate(x: string): string {
        return "invoke1"
    }
    protected fooProtected(x: number): string {
        return "invoke2"
    }
    public fooPublic(x: boolean): string {
        return "invoke3"
    }

    overload foo{ fooPrivate, fooProtected, fooPublic }

    test(){
        arktest.assertEQ(this.foo("abc"), "invoke1");
        arktest.assertEQ(this.foo(1), "invoke2");
        arktest.assertEQ(this.foo(true), "invoke3");
    }
}

class Sub extends Base {
    test() {
        arktest.assertEQ(this.foo(1), "invoke2");
        arktest.assertEQ(this.foo(true), "invoke3");
    }
}

function main() {
    let a = new Base();
    arktest.assertEQ(a.foo(true), "invoke3");
    a.test();

    let b = new Sub();
    b.test();
}
