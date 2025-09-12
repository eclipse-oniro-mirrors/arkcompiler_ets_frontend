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
    constructor withName(name: string) {}
    constructor withTask(name: string, task: ()=>void) {}
    overload constructor {withName, withTask}
}

class DummyArray<T> {
    pushArray(...val: T[]): void {}
    pushOne(value: T): void {}
    overload push { pushOne, pushArray }
}

let props: DummyArray<A> = new DummyArray<A>()

/* @@ label1 */props.push(
  /* @@ label2 */new A('eaw1', () => {
    let s: string = /* @@ label3 */1
  })
)

/* @@@ label1 Error TypeError: No matching call signature for push(...) */
/* @@@ label2 Error TypeError: No matching construct signature for A("eaw1", (() => void)) */
/* @@@ label3 Error TypeError: Type 'Int' cannot be assigned to type 'String' */
