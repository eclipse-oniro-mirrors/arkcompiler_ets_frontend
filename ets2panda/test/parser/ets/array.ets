/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

let a: int[] = [1,2,3];
let v: String[] = ["x", "y", "z"];

function add(arg : double[]): void {}

function foo(): void {
  add([0.1, 2.2, 3.3]);
}

function bar() : void {
    let a: byte[][] = [[], [], [], []];
    let b: byte[][] = new byte[4][4];
}

/* @@? 27:23 Error Semantic error ESE0318: Type 'Byte' cannot be assigned to type 'Array<Array<Byte>>' */
/* @@? 27:37 Error Syntax error ESY63526: Expression to initialize array element is missed. */
