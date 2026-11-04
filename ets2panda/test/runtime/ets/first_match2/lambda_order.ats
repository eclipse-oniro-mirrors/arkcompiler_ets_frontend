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

class Test {
    foo(lambda: () => void): string {
        return "invoke1";
    }
    foo(lambda: (p1: number, p2: string) => void): string {
        lambda(123456, "invoke2");
        return "invoke2";
    }
    foo(lambda: (p1: number, p2: string, p3: boolean) => void): string {
        lambda(123, "123", true);
        return "invoke3"
    }
}

function main() {
    let a: Test = new Test();
    arktest.assertEQ(a.foo((): void => { }), "invoke1");
    arktest.assertEQ(a.foo((p1: number, p2: string): void => { }), "invoke2");
    arktest.assertEQ(a.foo((p1: number, p2: string, p3: boolean): void => { }), "invoke3");

    let param1 = (): void => { };
    let param2 = (p1: number, p2: string): void => { };
    let param3 = (p1: number, p2: string, p3: boolean): void => { };
    arktest.assertEQ(a.foo(param1), "invoke1");
    arktest.assertEQ(a.foo(param2), "invoke2");
    arktest.assertEQ(a.foo(param3), "invoke3");
}
