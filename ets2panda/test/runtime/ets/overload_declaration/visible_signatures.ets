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
    foo11(a: Long): Int {
        return 1;
    }
    private foo12(a: Int): Int {
        return 0;
    }

    overload foo{ foo11, foo12 }

    foo21(a: Int): Int {
        return 1;
    }
    private foo22(a: Long): Int {
        return 0;
    }

    overload foo2{ foo21, foo22 }

    foo31(a: Long): Int {
        return 1;
    }
    private foo32(a: Int): Int {
        return 0;
    }

    overload foo3{ foo31, foo32 }

    foo41(a: Int): Int {
        return 1;
    }
    protected foo42(a: Long): Int {
        return 0;
    }

    overload foo4{ foo41, foo42 }
}

function main(): void {
    arktest.assertEQ(new A().foo(3), 1)
    arktest.assertEQ(new A().foo2(3), 1)
    arktest.assertEQ(new A().foo3(3), 1)
    arktest.assertEQ(new A().foo4(3), 1)
}
