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

let arr: readonly int[] = [1];

arr./* @@ extendTo */extendTo(2, 0);
arr./* @@ shrinkTo */shrinkTo(2);
arr./* @@ sort */sort();
arr./* @@ shift */shift();
arr./* @@ pop */pop();
arr./* @@ push */push(2, 3);
arr./* @@ splice */splice(0,1,2,3);
arr./* @@ unshift */unshift(-1);
arr./* @@ copyWithin */copyWithin(0,0,0);
arr./* @@ fill */fill(3);
arr./* @@ toSpliced */toSpliced();
arr./* @@ reverse */reverse();
arr./* @@ with */with(0, 5);

/* @@@ extendTo Error TypeError: Property 'extendTo' does not exist on type 'ReadonlyArray' */
/* @@@ shrinkTo Error TypeError: Property 'shrinkTo' does not exist on type 'ReadonlyArray' */
/* @@@ sort Error TypeError: Property 'sort' does not exist on type 'ReadonlyArray' */
/* @@@ shift Error TypeError: Property 'shift' does not exist on type 'ReadonlyArray' */
/* @@@ pop Error TypeError: Property 'pop' does not exist on type 'ReadonlyArray' */
/* @@@ push Error TypeError: Property 'push' does not exist on type 'ReadonlyArray' */
/* @@@ splice Error TypeError: Property 'splice' does not exist on type 'ReadonlyArray' */
/* @@@ unshift Error TypeError: Property 'unshift' does not exist on type 'ReadonlyArray' */
/* @@@ copyWithin Error TypeError: Property 'copyWithin' does not exist on type 'ReadonlyArray' */
/* @@@ fill Error TypeError: Property 'fill' does not exist on type 'ReadonlyArray' */
/* @@@ toSpliced Error TypeError: Property 'toSpliced' does not exist on type 'ReadonlyArray' */
/* @@@ reverse Error TypeError: Property 'reverse' does not exist on type 'ReadonlyArray' */
/* @@@ with Error TypeError: Property 'with' does not exist on type 'ReadonlyArray' */
