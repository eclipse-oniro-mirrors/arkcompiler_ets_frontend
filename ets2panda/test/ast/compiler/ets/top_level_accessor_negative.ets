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

native get nativeNoType()

native get nativeWithBody(): number { return 1 }

get noBodyGetter(): number 

set setterWithReturnType(x: number): void {}

get getterWithParam(x: number): number { return x }

/* @@? 16:12 Error Semantic error ESE0018: Native and Declare methods should have explicit return type. */
/* @@? 18:37 Error Semantic error ESE0083: Native, Abstract and Declare methods cannot have body. */
/* @@? 20:5 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 22:1 Error Syntax error ESY0241: Setter must not have return type even if it is void. */
/* @@? 24:1 Error Syntax error ESY0058: Getter must not have formal parameters. */