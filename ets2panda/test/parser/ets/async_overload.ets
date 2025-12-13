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

class Foo {
    async foo(i: int): Promise<Object | null> {
        return null;
    }

    async foo(o: Object | null, i: int): Promise<Object | null> {
        return null;
    }
}

async function bar(i: int): Promise<Object | null> {
    return null;
}

async function bar(o: Object | null, i: int): Promise<Object | null>{
    return null;
}

function main(): void {
    bar(0);
    bar(null, 0);
    let foo = new Foo();
    foo.foo(0);
    foo.foo(null, 0);
}
