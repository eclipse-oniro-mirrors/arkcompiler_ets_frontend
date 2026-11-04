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

class Foo<T> {
    then<U>(): Foo<U> | null {
        return null;
    }

    public value: T = undefined as T;
}

class A<T> {
    bar(): A<T> | null {
        return null;
    }

    baz(): Foo<T> | null{
        return null;
    }
}

function bar(p: Foo<Object>): void {
    let p1: Foo<Object> | null = p.then<Object>();
    let p2: Foo<A<A<Object>>> = new Foo<A<A<Object>>>();
    p1 = p2.value.bar()!.baz()!.then<Object>();
}
