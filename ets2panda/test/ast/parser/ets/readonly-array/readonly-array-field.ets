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
        this.arr./* @@ extendTo */extendTo(2, 0);
        this.arr./* @@ shrinkTo */shrinkTo(2);
        this.arr./* @@ sort */sort();
        this.arr./* @@ shift */shift();
        this.arr./* @@ pop */pop();
        this.arr./* @@ push */push(2, 3);
        this.arr./* @@ splice */splice(0,1,2,3);
        this.arr./* @@ unshift */unshift(-1);
        this.arr./* @@ copyWithin */copyWithin(0,0,0);
        this.arr./* @@ fill */fill(3);
        this.arr./* @@ toSpliced */toSpliced();
        this.arr./* @@ reverse */reverse();
        this.arr./* @@ with */with(0, 5);
    }
}

/* @@@ extendTo Error Semantic error ESE0087: Property 'extendTo' does not exist on type 'ReadonlyArray' */
/* @@@ shrinkTo Error Semantic error ESE0087: Property 'shrinkTo' does not exist on type 'ReadonlyArray' */
/* @@@ sort Error Semantic error ESE0087: Property 'sort' does not exist on type 'ReadonlyArray' */
/* @@@ shift Error Semantic error ESE0087: Property 'shift' does not exist on type 'ReadonlyArray' */
/* @@@ pop Error Semantic error ESE0087: Property 'pop' does not exist on type 'ReadonlyArray' */
/* @@@ push Error Semantic error ESE0087: Property 'push' does not exist on type 'ReadonlyArray' */
/* @@@ splice Error Semantic error ESE0087: Property 'splice' does not exist on type 'ReadonlyArray' */
/* @@@ unshift Error Semantic error ESE0087: Property 'unshift' does not exist on type 'ReadonlyArray' */
/* @@@ copyWithin Error Semantic error ESE0087: Property 'copyWithin' does not exist on type 'ReadonlyArray' */
/* @@@ fill Error Semantic error ESE0087: Property 'fill' does not exist on type 'ReadonlyArray' */
/* @@@ toSpliced Error Semantic error ESE0087: Property 'toSpliced' does not exist on type 'ReadonlyArray' */
/* @@@ reverse Error Semantic error ESE0087: Property 'reverse' does not exist on type 'ReadonlyArray' */
/* @@@ with Error Semantic error ESE0087: Property 'with' does not exist on type 'ReadonlyArray' */
