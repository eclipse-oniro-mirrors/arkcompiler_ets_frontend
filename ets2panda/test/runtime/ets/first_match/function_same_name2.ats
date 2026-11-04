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

type Callback<T, V = void> = (data: T) => V;

export class KeyEvent { }

namespace NS {
    export overload onKeyEvent{ onKeyEvent1, onKeyEvent0 };
    export function onKeyEvent0(event: (((event: KeyEvent) => void) | undefined)): string {
        return "namespace.invoke1";
    }

    export function onKeyEvent1(event: (Callback<KeyEvent, boolean> | undefined)): string {
        return "namespace.invoke2";
    }
}

overload onKeyEvent{ onKeyEvent1, onKeyEvent0 };
function onKeyEvent0(event: (((event: KeyEvent) => void) | undefined)): string {
    return "invoke1";
}

function onKeyEvent1(event: (Callback<KeyEvent, boolean> | undefined)): string {
    return "invoke2";
}

function main() {
    let test1 = onKeyEvent(((event: KeyEvent): boolean => {
        return false;
    }))
    arktest.assertEQ(test1, "invoke2")

    let test2 = onKeyEvent(((event: KeyEvent): void => {
    }))
    arktest.assertEQ(test2, "invoke1")

    let test3 = NS.onKeyEvent(((event: KeyEvent): boolean => {
        return false;
    }))
    arktest.assertEQ(test3, "namespace.invoke2")

    let test4 = NS.onKeyEvent(((event: KeyEvent): void => {
    }))
    arktest.assertEQ(test4, "namespace.invoke1")
}
