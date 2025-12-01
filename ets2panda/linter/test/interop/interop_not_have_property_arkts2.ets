/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

'use static'
import { foo, person, TestHelper, Machine, User, Person, Employee } from "./interop_not_have_property_js"

foo.name
foo.name = "456"
person.age = 23
person.male = [2, 3]
foo.age = 12
if (foo.name = "456") { print("true") }

let a = new foo()
a.age = 12

let test_helper = new TestHelper("TEST_INSTANTIATE_JS_OBJECT");
test_helper.test(() => {
    let machine = new Machine();
    return machine.name === "machine"; // arkts-interop-js2s-access-js-prop
}, "machine.name === 'machine'");

test_helper.test(() => {
    let user = new User("Bob");
    return user.id === "Bob"; // arkts-interop-js2s-access-js-prop
}, "user.id === 'Bob'");

test_helper.test(() => {
let user = new User(10);
return user.id === 10;// arkts-interop-js2s-access-js-prop
}, "user.id === 10");

test_helper.test(() => {
    let user = new User(123n);
    return user.id === 123n; // arkts-interop-js2s-access-js-prop
}, "user.id === 123n");

test_helper.test(() => {
    let user = new User(true);
    return user.id === true;// arkts-interop-js2s-access-js-prop
}, "user.id === true");

test_helper.test(() => {
    let machine = new Person("John", 10);
    return machine.name === "machine"; // arkts-interop-js2s-access-js-prop
}, "machine.name === 'machine'");

test_helper.test(() => {
    let employee = new Employee();
    return employee.name === "employee"; // arkts-interop-js2s-access-js-prop
}, "employee.name === 'employee'");