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

function goo()
{
   return new Int(3);
}

function foo(param? : Object)
{
   let var1? : Object;
   let var2? = new Object();
   let var3? = goo();
   for (let i? : Number = 1;;)  { break; }
}

/* @@? 23:14 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */
/* @@? 24:14 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */
/* @@? 25:14 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */
/* @@? 26:16 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */