/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

class B<T> {
    a: T;

    init(klazz: Class<T>) : void {
        a = klazz.newInstance();
    }
}

class A<T> {
    b: B<T>;

    init(klazz: Class<B<T>>) : void {
        b = klazz.newInstance();
    }
}

class Z {
    c: byte = 12;
}

function main(): void {
    let d = new A<Z>;
    d.init(B<Z>.class);
    d.b.init(Z.class);
    arktest.assertEQ(d.b.a.c, 12)
}
