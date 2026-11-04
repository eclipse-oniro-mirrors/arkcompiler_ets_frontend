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

function constantCases(): void{
    let a : char = Int.toChar(100);
    let first = true ? a : 68;

    let b : byte = 50;
    let sec = false ? b : 68;

    let c : Short = 721;
    let third = true ? c : 419;

    let d : char = Int.toChar(3);
    let fourth = false ? c : 665;

    let e : Byte = 88;
    let fifth = true ? c : 665;

    let f : short = 20;
    let sixth = true ? c : 665419;

    let g : double = 40;
    let seventh = false ? g : 30;
}

function unboxingCases(): void{
    let a : Char = c'k';
    let b : char = c'l';
    let first = true ? a : b;

    let c : Int = 11;
    let d : int = 10;
    let sec = false ? a : b;
}

function dominantNumericCases(): void{
    let a : double = 2.0;
    let b : float = 2.0f;
    let c : long = 2;
    let d : int = 2;
    let e : Short = 2;
    let f : byte = 2;
    let g : char = Int.toChar(2);

    let first = false ? a : b;
    let sec = false ? b : c;
    let third = false ? c : d;
    let fourth = false ? d : e;
    let fifth = false ? e : f;
    let sixth = false ? f : g;
}

function main(): void {
    constantCases();
    unboxingCases();
    dominantNumericCases();
}
