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

class A{
  field1:int;
  field2:int;
  method1(){};
}

function foo(c: "abcd"|A|(keyof A)):string {
  if (c == "field1")  {
    arktest.assertEQ(c, "field1")
    return "Case 1";
  } else if (c == "abcd") {
    arktest.assertEQ(c, "abcd")
    return "Case 2";
  } else if (c instanceof A) {
    arktest.assertEQ(c.field1, 0)
    return "Case 3";
  } else {
    return "Case 4";
  }
}

class Action{
  type:keyof A = "field1"
}

function reducer(action:Action):string{
  switch(action.type){
    case "field1":
      return "Case 1";
    case "field2":
      return "Case 2"
    case "method1":
      return "Case 3"
    default: 
      return "Case 4";
  }
}

function main(): void {
  arktest.assertEQ(foo("field1"), "Case 1");
  arktest.assertEQ(foo("abcd"), "Case 2");
  arktest.assertEQ(foo(new A()), "Case 3");
  
  arktest.assertEQ(reducer({type:"field1"}), "Case 1");
  arktest.assertEQ(reducer({type:"field2"}), "Case 2");
  arktest.assertEQ(reducer({type:"method1"}), "Case 3");
}
