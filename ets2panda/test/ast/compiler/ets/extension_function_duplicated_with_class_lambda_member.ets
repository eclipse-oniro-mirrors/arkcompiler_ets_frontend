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

class A{
    b : String = "b"
    foo : () => String
};

function foo(this:A)/* @@ label */{
    return this.b;
}

let a = new A;
a.foo = foo;
console.log(a.foo())

/* @@@ label Error TypeError: The extension accessor or extension function 'foo' has the same name with field of class A */
