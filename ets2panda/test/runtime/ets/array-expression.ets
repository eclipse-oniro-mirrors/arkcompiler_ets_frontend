/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function add(arg : double[]): double {
  return arg[0] + arg[1] + arg[2];
}

// TODO(user): unify copyBooleanArr and copyCharArr once generics are supported
function copyBooleanArr(src: boolean[]): boolean[] {
    let dst: boolean[] = new boolean[src.length];
    for (let i = 0; i < src.length; i++) {
        dst[i] = src[i];
    }
    return dst;
}

// TODO(user): unify copyBooleanArr and copyCharArr once generics are supported
function copyCharArr(src: char[]): char[] {
    let dst: char[] = new char[src.length];
    for (let i = 0; i < src.length; i++) {
        dst[i] = src[i];
    }
    return dst;
}

// TODO(user): unify cmpBooleanArr and cmpCharArr once generics are supported
function cmpBooleanArr(arg0: boolean[], arg1: boolean[]): boolean {
    if (arg0.length != arg1.length) {
        return false;
    }

    for (let i = 0; i < arg0.length; i++) {
        if (arg0[i] != arg1[i]) {
            return false;
        }
    }

    return true;
}

// TODO(user): unify cmpBooleanArr and cmpCharArr once generics are supported
function cmpCharArr(arg0: char[], arg1: char[]): boolean {
    if (arg0.length != arg1.length) {
        return false;
    }

    for (let i = 0; i < arg0.length; i++) {
        if (arg0[i] != arg1[i]) {
            return false;
        }
    }

    return true;
}

function main(): void {
  let arr : double[] = [1.1, 2.1, 3.5, 4.9];
  arktest.assertTrue(arr[0] == 1.1 && arr[1] == 2.1 && arr[2] == 3.5 && arr[3] == 4.9)
  arktest.assertEQ(add([1.1, 2.2, 3.3]), 6.6)

  let arr2 : boolean[] = [false, true, false, true, true, false, false, true];
  arktest.assertTrue(cmpBooleanArr(copyBooleanArr(arr2), arr2))

  let arr3 : char[] = [c'a', c'b', c'c', c'd'];
  arktest.assertTrue(cmpCharArr(copyCharArr(arr3), arr3))

  return;
}
