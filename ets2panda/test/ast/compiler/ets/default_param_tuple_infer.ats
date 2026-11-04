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

type TupleAlias = [ number, string ]

function handleTupleAlias_notCast(tuple: TupleAlias  = [ 1, 'string' ]) {}
function handleTupleAdHoc_notCast(tuple: [ number, string ] = [ 1, 'string' ]) {}
function handleByte_notCast(arg: byte = 1): byte { return arg; }

function handleTupleAlias_cast(tuple: TupleAlias  = [ 1, 'string' ] as TupleAlias) {}
function handleTupleAdHoc_cast(tuple: [ number, string ] = [ 1, 'string' ] as [ number, string ]) {}
function handleByte_cast(arg: byte = 1 as byte): byte { return arg; }
