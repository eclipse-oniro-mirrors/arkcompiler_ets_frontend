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

enum Color { Red, Green = -100, Blue, White = 0x00 }

function main(): void {
  let red = Color.Red./* @@ label */ordinal();
  let green = Color.Green./* @@ label1 */ordinal();
  let blue = Color.Blue./* @@ label2 */ordinal();	
  let white = Color.White./* @@ label3 */ordinal();
}


/* @@@ label Error Semantic error ESE0087: Property 'ordinal' does not exist on type 'Color' */
/* @@@ label1 Error Semantic error ESE0087: Property 'ordinal' does not exist on type 'Color' */
/* @@@ label2 Error Semantic error ESE0087: Property 'ordinal' does not exist on type 'Color' */
/* @@@ label3 Error Semantic error ESE0087: Property 'ordinal' does not exist on type 'Color' */