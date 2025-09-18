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

function main<T>(){
  foo(Class.from<Int| String | null | undefined >());
  foo(Class.from<String>());
  foo(Class.from<int>());
  foo(Class.from<undefined>());
  foo(Class.from<never>());
  foo(Class.from<T>());
  foo(Class.from<void>());
}

function foo(a:Class){}

/* @@? 20:3 Error TypeError: Type undefined cannot be used in the `from` method */
/* @@? 21:18 Error TypeError: Type never cannot be used in the `from` method */
/* @@? 22:18 Error TypeError: Type T cannot be used in the `from` method */
/* @@? 23:18 Error TypeError: Type void cannot be used in the `from` method */
