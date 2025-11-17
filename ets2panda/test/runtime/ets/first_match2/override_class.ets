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
    foo(a: number):string{
        return "invoke1";
    }
    foo(a: string):string{
        return "invoke2";
    }
}

class Sub1 extends Base {
    override foo(a: number): string {
        return "invoke3";
    }
}

class Sub2 extends Base {
    foo(a: int): string {
        return "invoke4";
    }
}

function main() {
    let a = new Sub1();
    arktest.assertEQ(a.foo(1), "invoke3")
    arktest.assertEQ(a.foo("abc"), "invoke2")

    let b = new Sub2();
    arktest.assertEQ(b.foo(1), "invoke4")
    arktest.assertEQ(b.foo("abc"), "invoke2")
}
