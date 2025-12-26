/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

enum E {
  a = 1, b = 2, c = 3
}

function foo(e: E, i: int) {
  i = e  // OK if widening, otherwise - CTE
  i = e as int  // CTE
  e = i  // CTE
  e = 1 as E  // CTE
  e = E.fromValue(i)  // RTE, if i is not equal to one of enum constants, otherwise OK
  e = 1  // CTE
}

enum SE {
  a = "a", b = "b", c = "c"
}

function foo2(e: SE, v: string) {
  v = e  // OK, implicitly to string
  v = e.valueOf()  // OK explicitly
  v = e as string  // CTE
  e = v  // CTE
  e = "c" as SE  // CTE
  e = SE.fromValue(v)  // RTE, if i is not equal to one of enum constants, otherwise OK
  e = "c"  // CTE
}

interface Message<T> {
  name: string;
  value: T;
}
type MessageData = string | number | boolean | E | SE

function foo3(e: E, se: SE, i: int, s: string, n: number, b: boolean) {
  let msg: Message<MessageData> = { name: "stringVal", value: "1" }
  let msg2: Message<MessageData> = { name: "numberVal", value: 1.0 };
  let msg3: Message<MessageData> = { name: "boolVal", value: true };
  let msg4: Message<MessageData> = { name: "enumVal", value: E.c };
  let msg5: Message<MessageData> = { name: "strEnumVal", value: SE.c };
  
  // check valid convertions from unions, no CTE & RTE
  s = msg.value as string
  n = msg2.value as number
  b = msg3.value as boolean
  e = msg4.value as E
  se = msg5.value as SE

  // check flags
  e = E.fromValue( E.a | E.b )
}

/* @@? 22:7 Error Semantic error ESE593372: Conversion from 'E' to 'Int' is not supported. Please use implicit conversion of enum value to 'Int' value or call 'enumValue.valueOf()' */
/* @@? 23:7 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'E' */
/* @@? 24:7 Error Semantic error ESE593373: Conversion from 'Int' to 'E' is not supported. Please use 'EnumType.fromValue(value)' */
/* @@? 26:7 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'E' */
/* @@? 36:7 Error Semantic error ESE593372: Conversion from 'SE' to 'String' is not supported. Please use implicit conversion of enum value to 'String' value or call 'enumValue.valueOf()' */
/* @@? 37:7 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'SE' */
/* @@? 38:7 Error Semantic error ESE593373: Conversion from '"c"' to 'SE' is not supported. Please use 'EnumType.fromValue(value)' */
/* @@? 40:7 Error Semantic error ESE0318: Type '"c"' cannot be assigned to type 'SE' */
