/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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
    test(c: B<T>): B<T> {
        return c;
    }

    test2(c: B<T>): B<T> {
        return this;
    }
}

class A<T> {
    bar(): A<T> {
        return this;
    }

    baz(): C<T> | null {
        return null;
    }
}

class C<T> {
    then<U>(a0: U): C<U> | null {
        return null;
    }

    public value: T = undefined as T;
}

function bar(p: C<Object>): void {
    let b_Int: B<Int> = new B<Int>();
    b_Int.test(b_Int);

    let p1: C<Object> | null = p.then<Object>(new Object());
    let p2: C<A<A<Object>>> = new C<A<A<Object>>>();
    p1 = p2.then<Object>(new Object());
    p1 = p2.value.bar().baz()!.then<Object>(new Object());
    p1 = p2.value.bar().baz()!.then<Object>(new Object());

    let p3: C<Double> | null = p.then<Double>(new Double());
    let p4: C<A<A<Double>>> = new C<A<A<Double>>>();
}
