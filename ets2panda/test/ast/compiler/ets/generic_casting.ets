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
    static s_instance: Foo | undefined;
    value: boolean = false;

    static foo<T extends Foo>(): void {
        const instance = Foo.s_instance as T;
        if (instance != undefined) {
            instance.value = true;
        }
    }
}

class Bar extends Foo { }
Foo.s_instance = new Bar();
Foo.foo<Bar>();
arktest.assertTrue(Foo.s_instance?.value == true);
arktest.assertTrue((Foo.s_instance as Bar).value);
