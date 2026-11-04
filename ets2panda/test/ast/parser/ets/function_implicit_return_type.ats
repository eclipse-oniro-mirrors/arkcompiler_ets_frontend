
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

// Intended formatting for source position testing
function void1(i: int)     {}
let void2: (i: int) => void = void1;
let void3: (i: int)  => void  = (i: int): void => { return; };
let void4: (i: int) =>  void = (  i: int)      => { return; };
function void5(fn1: () => void, fn2: (x: int) => void) { fn1(); fn2(2); }
let void6: (i: int  )       /* @@ label */= void1 /* @@ label1 */; // CTE; need return type

function main() {}


/* @@@ label Error Syntax error ESY0228: Unexpected token, expected '=>'. */
/* @@@ label Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 23:45 Error Syntax error ESY0227: Unexpected token 'void1'. */