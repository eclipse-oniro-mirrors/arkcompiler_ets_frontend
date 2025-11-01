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

let O = new Object();
let I = new Int(3);

class C 
{
   field? : Object;
   method(s: string) : int 
   {
      return 1;
   }

   method (s? : string) : int
   { 
      return 2;
   }
}

function goo()
{
   return I;
}

function main() : int
{
   let c = new C();
   let var1? : Object;
   let var2? = O;
   let var3? = goo();

   arktest.assertEQ(c.method('blablabla'), 1)
   arktest.assertEQ(c.method(undefined), 2)
   arktest.assertEQ(c.method(), 2)
   arktest.assertTrue(c.field === undefined)

   arktest.assertTrue(var1 === undefined)
   arktest.assertTrue(var2 === O)
   arktest.assertTrue(var3 === I)

   return 0;
}

/* @@? 27:4 Error TypeError: Function method with this assembly signature already declared. */
/* @@? 41:14 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 42:14 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 43:14 Error SyntaxError: Optional variable is deprecated and no longer supported. */