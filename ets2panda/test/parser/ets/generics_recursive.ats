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

class A<TA> extends B<TA> {}

class B<TB> {
    f() {}
    g(a: A<TB>) {
        a.f();
    }
}

class M<K, V> {
    constructor(iter: ArrayLike<[K, V]> | Iterable<[K, V]>) {
        Array.from<[K, V]>(iter);
    }
}

class C<T> {}

function mp<T>(): C<T> {
    return new C<T>();
}

class M1<K, V> {
    it(): C<[K, V]> {
        return this.bar();
    }
    bar(): C<[K, V]> {
        return mp<[K, V]>();
    }
}
