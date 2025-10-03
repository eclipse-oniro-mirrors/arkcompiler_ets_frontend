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

function getFuncWithArgsZero(func: (() => void) | (() => Promise<void>)) {
    return new Promise<void>(async (resolve: (value: PromiseLike<void>) => void) => {
        await (func as () => Promise<void>)();
        const funcStr = Type.of(func as object).getLiteral()
        const argsCount = getFunctionArgumentsCount(funcStr)
        const isAsync = checkIsAsyncFunction(funcStr)
        resolve(Promise.resolve());
    });
}

function getFunctionArgumentsCount(funcStr: string): number {
    const regex = new RegExp("^[0123456789]$", "g")
    let count = ""
    for(let ch of funcStr) {
        let str = new Char(ch).toString();
        if(regex.test(str)) {
            count = str
        }
    }
    if(count.length) {
        return Number(count)
    }
    return 0
}

function checkIsAsyncFunction(funcStr: string): boolean {
    const endIndex = funcStr.lastIndexOf("):")
    const validStr = funcStr.slice(endIndex)
    if(validStr.indexOf("Promise") !== -1) {
        return true
    } else {
        return false
    }
}

async function testAsyncGetFuncWithArgsZeroSafe() {
    let success = false;
    try {
        await getFuncWithArgsZero( async () => {});
        success = true;
    } catch (e) {
        success = false;
    }
    arktest.assertEQ(success, true, "getFuncWithArgsZero with async should not throw");
}

function testGetFuncWithArgsZeroSafe() {
    let success = false;
    try {
        getFuncWithArgsZero(() => {});
        success = true;
    } catch (e) {
        success = false;
    }
    arktest.assertEQ(success, true, "getFuncWithArgsZero should not throw");
}

function main(): void {
    const func1 = (a: number, b: number): number => {
        return a + b;
    };

    const func2 = async (): Promise<void> => {};

    const func3 = (): number => {
        return 42;
    };

    const func4 = async (x: number, y: number, z: number): Promise<void> => {};

    const funcStr1: string = Type.of(func1 as object).getLiteral();
    const funcStr2: string = Type.of(func2 as object).getLiteral();
    const funcStr3: string = Type.of(func3 as object).getLiteral();
    const funcStr4: string = Type.of(func4 as object).getLiteral();

    // Test getLiteral()
    arktest.assertEQ(funcStr1, "(1: std.core.Double, 2: std.core.Double): std.core.Double", "func1 literal check");
    arktest.assertEQ(funcStr2, "(): std.core.Promise", "func2 literal check");
    arktest.assertEQ(funcStr3, "(): std.core.Double", "func3 literal check");
    arktest.assertEQ(funcStr4, "(1: std.core.Double, 2: std.core.Double, 3: std.core.Double): std.core.Promise", "func4 literal check");

    // Test getFunctionArgumentsCount
    arktest.assertEQ(getFunctionArgumentsCount(funcStr1), 2, "func1 should have 2 arguments");
    arktest.assertEQ(getFunctionArgumentsCount(funcStr2), 0, "func2 should have 0 arguments");
    arktest.assertEQ(getFunctionArgumentsCount(funcStr3), 0, "func3 should have 0 arguments");
    arktest.assertEQ(getFunctionArgumentsCount(funcStr4), 3, "func4 should have 3 arguments");

    // Test checkIsAsyncFunction
    arktest.assertEQ(checkIsAsyncFunction(funcStr1), false, "func1 should not be async");
    arktest.assertEQ(checkIsAsyncFunction(funcStr2), true, "func2 should be async");
    arktest.assertEQ(checkIsAsyncFunction(funcStr3), false, "func3 should not be async");
    arktest.assertEQ(checkIsAsyncFunction(funcStr4), true, "func4 should be async");

    // execute getFuncWithArgsZero with async
    testAsyncGetFuncWithArgsZeroSafe()

    // execute getFuncWithArgsZero with sync
    testGetFuncWithArgsZeroSafe()
}


/* @@? 17:36 Error Semantic error ESE0046: Type '((p1: ((p1: PromiseLike<void>) => void)) => Promise<void>)' is not compatible with type '((p1: ((p1: void|PromiseLike<void>) => void), p2: ((p1: Error) => void)) => void)' at index 1 */
/* @@? 17:36 Warning Warning W49710: No matching signature found for Promise.then/catch: ((p1: ((p1: PromiseLike<void>) => void)) => Promise<void>) */
/* @@? 30:19 Error Semantic error ESE0124: Expected 0 arguments, got 1. */
/* @@? 30:19 Error Semantic error ESE0127: No matching construct signature for std.core.Char(String) */
/* @@? 30:28 Error Semantic error ESE0046: Type 'String' is not compatible with type 'Char' at index 1 */