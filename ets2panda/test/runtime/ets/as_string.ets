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


function cast2str<T>(v: T): string {
    return v as string;
}

class CastErrorTest {
    public suite: arktest.ArkTestsuite
    constructor(name: string) {
        this.suite = new arktest.ArkTestsuite(name)
    }
    public pass<T>(name: string, from: T, expect?: string) {
        const fromType = Type.of(from).toString();
        this.suite.addTest(`pass<${name}>(${fromType})`, () => {
            const value = cast2str(from);
            arktest.assertEQ(Type.of(value).toString(), Type.of("").toString());
            if (expect) {
                arktest.assertEQ(value, expect);
            }
        });
    }
    public fail<T>(name: string, from: T): void {
        const fromType = Type.of(from).toString();
        const errorFrom = from === null
            ? 'std.core.__NullValue'
            : fromType;
        const errorTo: string = from === undefined
            ? 'String'
            : 'std.core.String';
        this.suite.addTest(`fail<${name}>(${fromType})`, () => {
            arktest.expectError(
                () => { cast2str(from); },
                new ClassCastError(`${errorFrom} cannot be cast to ${errorTo}`)
            );
        });
    }
    public run() {
        this.suite.run();
    }
}

let tests = new CastErrorTest("AsString");

tests.pass<string|string[]>("string|string[]", "1,2", "1,2");
tests.fail<string|string[]>("string|string[]", ["1", "2"]);
tests.fail<string[]>("string[]", ["1", "2"]);
tests.fail<int|int[]>("int|int[]", [1, 2]);
tests.fail<int|int[]|string[]>("int|int[]|string[]", [1, 2]);
tests.fail<null|string|string[]>("null|string|string[]", null);
tests.fail<undefined|string|string[]>("undefined|string|string[]", undefined);
tests.pass<undefined|string|string[]>("undefined|string|string[]", "123");

tests.suite.addTest("+", () => {
    const ar = [1,2,3];
    arktest.assertEQ("Test " + ar, "Test " + ar.toString());
});

tests.suite.addTest("+", () => {
    const v: Double = 1.0;
    arktest.assertEQ("Test " + v, "Test " + v.toString());
});

tests.run();
