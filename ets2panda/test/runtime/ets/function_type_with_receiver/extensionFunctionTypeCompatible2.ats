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

class Base {}
class Derived extends Base {}
type FuncTypeBaseBase = (this: Base) => Base
type FuncTypeBaseDerived = (this: Base) => Derived
type FuncTypeDerivedBase = (this: Derived) => Base
type FuncTypeDerivedDerived = (this: Derived) => Derived

function BB(this: Base) : Base { return new Base() }
function BD(this: Base) : Derived { return new Derived() }
function DB(this: Derived) : Base { return new Base() }

let bb:FuncTypeBaseBase = BB;
let bd:FuncTypeBaseDerived = BD;
let db:FuncTypeDerivedBase = DB;

bb = bd;
db = bd
