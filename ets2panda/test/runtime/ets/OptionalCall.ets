
/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class Bar {
    cb: (() => int) | undefined = () => { return 3 };
}

function foo(a: Int | null): (() => Int) | undefined {
    let cb: () => Int = () => { return a!; };
    return a != null ? cb : undefined;
}

function main(): void {
    let a = new Int(3);
    // arktest.assertEQ(foo(a)?.(), 3 // bug)
    arktest.assertEQ(foo(a)?.(), a)

    arktest.assertTrue(foo(null)?.() == null)
    arktest.assertTrue(null == foo(null)?.())

    arktest.assertEQ(foo(null)?.(), undefined)
    arktest.assertEQ(undefined, foo(null)?.())

    arktest.assertNE(new Int(3), foo(null)?.())

    let b = new Bar();
    arktest.assertEQ(b.cb?.(), 3)
    b.cb = undefined;
    arktest.assertEQ(b.cb?.(), undefined)
}
