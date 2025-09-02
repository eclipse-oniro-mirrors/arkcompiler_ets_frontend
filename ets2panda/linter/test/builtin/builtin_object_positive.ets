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
const str: string = 'abc';
str.replace('a', 'b');
str.replaceAll('a', 'b');
class Demo implements String{ //BuiltinFinalClass
  replaceAll1(searchValue: string | RegExp, replacer: string): string {

  }
}
//replace(searchValue: string | RegExp, replaceValue: string): string;
const text = "Hello World, World!";
const result1 = text.replace("World", "TypeScript");
const result2 = text.replace(/World/g, "TypeScript");

//lib.es2015.symbol.wellknown.d.ts
const result6 = text.replace(/-/g, (match, offset:string) => `-${offset}-`);
const camelCase = text.replace(/_(\w)/g, (_, char:string) => char.toUpperCase());