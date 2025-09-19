
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

function fun(x: int)  { 
  let z  = c'Д';
  let y: double = 7.7;

  if (x > 0) {
    return z;  
  }

  if (x == 0) {
    return y;
  }

  return -33
}

function main() 
{
   let x = fun(2);
   let y = fun(0);
   let z = fun(-1);

   let cons = Console.getInstance();
   cons.print("x = " + x + "\n");
   cons.print("y = " + y + "\n");
   cons.print("z = " + z + "\n");
}
