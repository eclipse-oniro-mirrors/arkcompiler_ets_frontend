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

class A {}
function test(this: A[][]) {}

/* @@? 17:1 Node {
  "type": "MethodDefinition",
  "kind": "extensionmethod",
  "accessibility": "public",
  "static": true
} */

/* @@? 17:10 Node {
  "type": "Identifier",
  "name": "A"
} */

/* @@? 17:11 Node {
  "type": "TSArrayType"
} */

/* @@? 17:13 Node {
  "type": "TSArrayType"
} */

/* @@? 17:16 Node {
  "type": "Identifier",
  "name": "test"
} */

/* @@? 17:21 Node {
  "type": "Identifier",
  "name": "=t"
} */

/* @@? 17:23 Node {
  "type": "BlockStatement",
  "statements": []
} */
