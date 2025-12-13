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

type ErrorCallBack1<R = Object, T extends Object = Error> = (err: T) => R

type ErrorCallBack2<R, T extends Object> = (err: T) => R

type ErrorCallBack3<R, T extends Object = Error> = (err: T) => R

function test1(call: ErrorCallBack1) { } // should be ok

function test2(call: ErrorCallBack2) { } // should be error because typeParam has no default type

function test3(call: ErrorCallBack2<Object>) { } // should be error because we have not provided all types.

function test4(call: ErrorCallBack2<Object, Error>) { } // should be ok because we have provided all types.

function test5(call: ErrorCallBack3<Object>) { } // should be ok because the second type param has a default type

function test6(call: ErrorCallBack3) { } // should be error

/* @@? 24:22 Error Semantic error ESE0155: Type alias declaration is generic, but too few type arguments were provided */
/* @@? 26:36 Error Semantic error ESE0274: Expected at least 2 type arguments, but got 1. */
/* @@? 32:22 Error Semantic error ESE0155: Type alias declaration is generic, but too few type arguments were provided */