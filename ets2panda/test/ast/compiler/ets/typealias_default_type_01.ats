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

type ErrorCallBack1<T extends Object = Error> = (err: T) => void

type ErrorCallBack2<T extends Object> = (err: T) => void

function test1(call: ErrorCallBack1) { } // should be ok

function test2(call: ErrorCallBack2) { } // should be error because typeParam has no default type

function test3(call: ErrorCallBack2<Error>) { } // should be ok because we have explicitly provided a type.

/* @@? 22:22 Error Semantic error ESE0155: Type alias declaration is generic, but too few type arguments were provided */