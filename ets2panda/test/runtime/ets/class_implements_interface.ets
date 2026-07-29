/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

export interface I
{
    name :string | undefined
    numeric : boolean | undefined
}

export class C implements I
{
    name : string | undefined = "name"
    numeric : boolean | undefined = false
}

export interface I2
{
    name :string | undefined
    numeric : boolean | undefined
}

export class C2 implements I2
{
    name : string | undefined  = "name"
    numeric : boolean | undefined = false
}

export interface I3
{
    name :string
    numeric : boolean
}

export class C3 implements I3
{
    name : string = "name"
    numeric : boolean = false
}

export interface I4
{
    name :string
    numeric : boolean
}

export class C4 implements I4
{
    name : string = "name"
    numeric : boolean = false
}

function main(): void {
    let x:C = new C();
    let x2:C2 = new C2();
    let x3:C3 = new C3();
    let x4:C4 = new C4();
}
