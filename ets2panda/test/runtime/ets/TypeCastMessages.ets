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

function testExc(fn: () => Any, src: string, dst: string) {
    try {
        fn()
    } catch (e) {
        if (!(e instanceof ClassCastError)) {
            throw new Error("wrong type")
        }
        arktest.assertEQ(e.message, src + " cannot be cast to " + dst)
        return;
    }
    throw new Error("nothing was thrown")
}

function getName(v: Any) {
    return Class.of(v as object | null).getName()
}

class Something { }

function test(v: Any) {
    const vname = getName(v)
    const orUndefined = " or undefined"

    testExc(() => v as undefined, vname, "undefined")
    testExc(() => v as never | undefined, vname, "undefined")
    testExc(() => v as null, vname, getName(null))
    testExc(() => v as null | undefined, vname, getName(null) + orUndefined)

    testExc(() => v as string, vname, getName(new String()))
    testExc(() => v as "aaa!", vname, getName(new String()))

    testExc(() => v as Something, vname, getName(new Something()))

    // testExc(() => v as string | Something, vname, Class.from<string | Something>().getName()) #31354
}

function testUndefined(v: Any) {
    testExc(() => v as null, "undefined", getName(null))
    testExc(() => v as Something, "undefined", getName(new Something()))
}

class Poison { }
test(new Poison())
testUndefined(undefined)
