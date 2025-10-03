/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function fn1<T extends D, D>(x: T, y: D) {}
fn1("123", "456")

function fn2<T extends D, D>(x: D, y: T) {}
fn2("123", "456")

function fn3<T, D extends T>(x: T, y: D) {}
fn3("123", "456")

function fn4<T, D extends T>(x: D, y: T) {}
fn4("123", "456")

function fn5<T extends D ,D>(x: D, y:T) {}
let v5 : string | number = 1
fn5(v5,"456")

function fn6<T, F extends T, D extends F>(x:T,y:D,z:F){} 
fn6("123","456","321")

function fn7<T extends D, D extends /* @@ label */T>(x:T, y:D){}
fn7("123","456")


/* @@@ label Error Semantic error ESE0184: Type parameter 'D' has circular constraint dependency. */