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

function foo(x: int): string
{
  let rc: string|undefined = "default";

  label1: switch(x) {
    case 0: 
       rc = "case 0";
    case 1: {
       let rc1: string|undefined = ():string => {return "case 1";}();
       label2: switch(rc) {
          case "default":
             rc = undefined;
             break label2;
          default:
             break label1;
       }
       rc = rc1;
       break;
    }
    case 2: 
       rc = undefined; 
       let rc2: string|null = ():string => {return "case 2";}();
       return rc2; 
    case 3: 
       rc = "case 3";
       break; 
    case 4:
       rc = undefined;
    default:
       return rc != null ? rc :"case 4"
    case 5:
       rc = "case 5";
  }

  return rc;
}


function main(): void {
  arktest.assertEQ(foo(0), "case 0")
  arktest.assertEQ(foo(1), "case 1")
  arktest.assertEQ(foo(2), "case 2")
  arktest.assertEQ(foo(3), "case 3")
  arktest.assertEQ(foo(4), "case 4")
  arktest.assertEQ(foo(5), "case 5")
  arktest.assertEQ(foo(7), "default")
}
