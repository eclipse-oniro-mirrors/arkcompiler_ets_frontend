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
  field1:number = 10;
  private field2:number = 20;
  foo(){}
}

function foo(c: (keyof A)|"abcd"|A|undefined): string {
  if (c instanceof (keyof A))  {
    arktest.assertEQ(c, "field1")
    return "Case 1";
  } else if (c instanceof /* @@ label1 */"abcd") {
    arktest.assertEQ(c, "abcd")
    return "Case 2";
  } else if (c instanceof A) {
    arktest.assertEQ(c.field1, 0)
    return "Case 3";
  } else {
    return "Case 4";
  }
}

function main(): void {
  /* @@ label2 */foo(/* @@ label3 */"field2");
  /* @@ label4 */foo(/* @@ label5 */"other field");
}

/* @@@ label1 Error Semantic error ESE38251: Type '"abcd"' cannot be used on the right hand side of an 'instanceof' expression */
/* @@@ label2 Error Semantic error ESE0127: No matching call signature for foo("field2") */
/* @@@ label3 Error Semantic error ESE0046: Type '"field2"' is not compatible with type '"abcd"|A|undefined|"foo"|"field1"' at index 1 */
/* @@@ label4 Error Semantic error ESE0127: No matching call signature for foo("other field") */
/* @@@ label5 Error Semantic error ESE0046: Type '"other field"' is not compatible with type '"abcd"|A|undefined|"foo"|"field1"' at index 1 */
