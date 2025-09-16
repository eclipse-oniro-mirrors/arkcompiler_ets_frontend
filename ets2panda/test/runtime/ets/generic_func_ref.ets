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

function genFunRetFunVoid<S>(): (y: S) => void {
  return (y: S): void => {};
}

function genFunRetFunS<S>(): (y: S) => S {
  return (y: S): S => { return y; };
}

function genFun<S>(a0: S): S {
  return a0;
}

function paramRetFunVoid(f: () => ((y: Int) => void)): int {
    let fRes = f();
    fRes(5);
    return 1;
}

function paramRetFunParam(f: () => ((y: Int) => Int)): int {
    let fRes = f();
    let finRes = fRes(4);
    arktest.assertEQ(finRes, 4);
    return 2;
}

function paramFun(f: (y: Int) => Int): int {
    let fRes = f(7);
    arktest.assertEQ(fRes, 7);
    return 3;
}

class A {
    genFunRetFunVoid<S>(): (y: S) => void {
        return (y: S): void => {};
    }

    genFunRetFunS<S>(): (y: S) => S {
        return (y: S): S => { return y; };
    }

    genFun<S>(a0: S): S {
        return a0;
    }

    paramRetFunVoid(f: () => ((y: Int) => void)): int {
        let fRes = f();
        fRes(5);
        return 4;
    }

    paramRetFunParam(f: () => ((y: Int) => Int)): int {
        let fRes = f();
        let finRes = fRes(8);
        arktest.assertEQ(finRes, 8);
        return 5;
    }

    paramFun(f: (y: Int) => Int): int {
        let fRes = f(9);
        arktest.assertEQ(fRes, 9);
        return 6;
    }
}

function paramRetFunVoidGen<T>(f: () => ((y: T) => void), f1: T): int {
    let fRes = f();
    fRes(f1);
    return 7;
}

function paramRetFunParamGen<T>(f: () => ((y: T) => T), f1: T): int {
    let fRes = f();
    let finRes = fRes(f1);
    arktest.assertEQ(finRes, 12);
    return 8;
}

function paramFunGen<T>(f: (y: T) => T, f1: T): int {
    let fRes = f(f1);
    arktest.assertEQ(fRes, 13);
    return 9;
}

function main(): void {
    let test1 = paramRetFunVoid(genFunRetFunVoid);
    arktest.assertEQ(test1, 1)

    let test2 = paramRetFunParam(genFunRetFunS);
    arktest.assertEQ(test2, 2)

    let test3 = paramFun(genFun)
    arktest.assertEQ(test3, 3)


    let aClass = new A();

    let test4 = aClass.paramRetFunVoid(aClass.genFunRetFunVoid);
    arktest.assertEQ(test4, 4)

    let test5 = aClass.paramRetFunParam(aClass.genFunRetFunS);
    arktest.assertEQ(test5, 5)

    let test6 = aClass.paramFun(aClass.genFun)
    arktest.assertEQ(test6, 6)

    let test7 = paramRetFunVoidGen<Int>(genFunRetFunVoid, 11);
    arktest.assertEQ(test7, 7)

    let test8 = paramRetFunParamGen<Int>(genFunRetFunS, 12);
    arktest.assertEQ(test8, 8)

    let test9 = paramFunGen<Int>(genFun, 13)
    arktest.assertEQ(test9, 9)
}
