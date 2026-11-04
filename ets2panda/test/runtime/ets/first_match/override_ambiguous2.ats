/**
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

let testString: string = "";

type Callback<T, V = void> = (data: T) => V;

export class KeyEvent { }

export class CommonMethod {
    overload onKeyEvent{ onKeyEvent1, onKeyEvent0 };
    onKeyEvent0(event: (((event: KeyEvent) => void) | undefined)): this{
        testString = "invoke1";
        return this;
    }
    onKeyEvent1(event: (Callback<KeyEvent, boolean> | undefined)): this{
        testString = "invoke2";
        return this;
    }
    background(): this{
        return this;
    }
}

export class CommonMethodImpl extends CommonMethod {
    onKeyEvent0(event: (((event: KeyEvent) => void) | undefined)): this {
        testString = "invoke3";
        return this;
    }

    onKeyEvent1(event: (Callback<KeyEvent, boolean> | undefined)): this {
        testString = "invoke4";
        return this;
    }
    background(): this { return this }
}

function test1(a: CommonMethodImpl) {
    a.background().onKeyEvent(((event: KeyEvent): void => {
    })).onKeyEvent(((event: KeyEvent): boolean => {
        return false;
    }))
}

function test2(a: CommonMethodImpl) {
    a.background().onKeyEvent(((event: KeyEvent): void => {
    }))
}

function test3(a: CommonMethod) {
    a.background().onKeyEvent(((event: KeyEvent): void => {
    })).onKeyEvent(((event: KeyEvent): boolean => {
        return false;
    }))
}

function test4(a: CommonMethod) {
    a.background().onKeyEvent(((event: KeyEvent): void => {
    }))
}

function main() {
    test1(new CommonMethodImpl());
    arktest.assertEQ(testString, "invoke4");
    test2(new CommonMethodImpl());
    arktest.assertEQ(testString, "invoke3");
    test3(new CommonMethodImpl());
    arktest.assertEQ(testString, "invoke4");
    test4(new CommonMethodImpl());
    arktest.assertEQ(testString, "invoke3");

    test3(new CommonMethod());
    arktest.assertEQ(testString, "invoke2");
    test4(new CommonMethod());
    arktest.assertEQ(testString, "invoke1");
}
