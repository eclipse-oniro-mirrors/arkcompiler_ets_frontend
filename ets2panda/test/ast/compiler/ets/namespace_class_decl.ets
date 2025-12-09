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

namespace testing {
    export class Array<T> {
        $_get(index: number): Array<T> {return this;}
        $_set(index: number, value: Array<T>) {}
    }
    export class ReadonlyArray<T> {
        $_get(index: number): ReadonlyArray<T> {return this;}
        $_set(index: number, value: ReadonlyArray<T>) {}
    }
    export class ConcatArray<T> {
            this.tabInfo.recentCount--;
          
}
    export class ArrayLike<T> {
        $_get(index: number): ArrayLike<T> {return this;}
        $_set(index: number, value: ArrayLike<T>) {}
    }
}

namespace testing2 {
    export class Array<T> {[x: number]: any}
    export class ReadonlyArray<T> {[x: number]: any}
    export class ConcatArray<T> {[x: number]: any}
    export class ArrayLike<T> {[x: number]: any}
}

function retA() {
    let a: Array<number> = [1,2,3];
    let b: ReadonlyArray<number> = [1,2,3];
    let c: ConcatArray<number> = [1,2,3];
    let d: ArrayLike<number> = [1,2,3];
}

function retB() {
    let a = new testing.Array<number>();
    let b = new testing.ReadonlyArray<number>();
    let c = new testing.ConcatArray<number>();
    let d = new testing.ArrayLike<number>();
}

function retC() {
    let a = new testing2.Array<number>();
    let b = new testing2.ReadonlyArray<number>();
    let c = new testing2.ConcatArray<number>();
    let d = new testing2.ArrayLike<number>();
}


/* @@? 26:13 Error Syntax error ESY0227: Unexpected token 'this'. */
/* @@? 26:17 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 26:25 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 26:25 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 26:37 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 26:37 Error Syntax error ESY0227: Unexpected token '--'. */
/* @@? 36:29 Error Semantic error ESE0343: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 37:37 Error Semantic error ESE0343: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 38:35 Error Semantic error ESE0343: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 39:33 Error Semantic error ESE0343: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 45:34 Error Semantic error ESE0318: Type 'Array<Int>' cannot be assigned to type 'ConcatArray<Double>' */
/* @@? 46:32 Error Semantic error ESE0318: Type 'Array<Int>' cannot be assigned to type 'ArrayLike<Double>' */