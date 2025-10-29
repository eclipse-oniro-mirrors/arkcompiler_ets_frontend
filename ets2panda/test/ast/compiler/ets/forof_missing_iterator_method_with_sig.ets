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

class C {
  $_iterator(param: int): string {
    return "notAnIterator"; 
  }
}

function main(): int {
  let c = new C();
  let res = "";

  for (let x of c) { 
    res += x;
  }

  if (res != "6") return 1;
  return 0;
}

/* @@? 17:3 Error Syntax error ESY0223: The special predefined method '$_iterator' should not have parameters. */
/* @@? 17:13 Error Semantic error ESE0096: The return type of '$_iterator' must be a type that implements Iterator interface. */
/* @@? 26:17 Error Semantic error ESE0259: Cannot find iterator method with the required signature. */
/* @@? 26:17 Error Semantic error ESE0076: 'For-of' statement source expression is not of iterable type. */