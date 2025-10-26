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

class A {
    arr: readonly int[] = [1];
    foo() {
        this.arr.extendTo(2, 0);
        this.arr.shrinkTo(2);
        this.arr.sort();
        this.arr.shift();
        this.arr.pop();
        this.arr.push(2, 3);
        this.arr.splice(0,1,2,3);
        this.arr.unshift(-1);
        this.arr.copyWithin(0,0,0);
        this.arr.fill(3);
        this.arr.toSpliced();
        this.arr.reverse();
        this.arr.with(0, 5);
    }
}

/* @@? 19:18 Error Semantic error ESE0087: Property 'extendTo' does not exist on type 'ReadonlyArray' */
/* @@? 20:18 Error Semantic error ESE0087: Property 'shrinkTo' does not exist on type 'ReadonlyArray' */
/* @@? 21:18 Error Semantic error ESE0087: Property 'sort' does not exist on type 'ReadonlyArray' */
/* @@? 22:18 Error Semantic error ESE0087: Property 'shift' does not exist on type 'ReadonlyArray' */
/* @@? 23:18 Error Semantic error ESE0087: Property 'pop' does not exist on type 'ReadonlyArray' */
/* @@? 24:18 Error Semantic error ESE0087: Property 'push' does not exist on type 'ReadonlyArray' */
/* @@? 25:18 Error Semantic error ESE0087: Property 'splice' does not exist on type 'ReadonlyArray' */
/* @@? 26:18 Error Semantic error ESE0087: Property 'unshift' does not exist on type 'ReadonlyArray' */
/* @@? 27:18 Error Semantic error ESE0087: Property 'copyWithin' does not exist on type 'ReadonlyArray' */
/* @@? 28:18 Error Semantic error ESE0087: Property 'fill' does not exist on type 'ReadonlyArray' */
/* @@? 29:18 Error Semantic error ESE0087: Property 'toSpliced' does not exist on type 'ReadonlyArray' */
/* @@? 30:18 Error Semantic error ESE0087: Property 'reverse' does not exist on type 'ReadonlyArray' */
/* @@? 31:18 Error Semantic error ESE0087: Property 'with' does not exist on type 'ReadonlyArray' */
