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

declare type type_array_1<T> = Array<T>;
declare type type_array_2<T> = T[];
declare type type_array_alias<T> = Array<type_array_1<T>> | type_array_2<T>

let a : int[] = [1,2,3]
let b : type_array_alias<string>  = a;




/* @@? 21:37 Error Semantic error ESE0318: Type 'Array<Int>' cannot be assigned to type 'Array<Array<String>>|Array<String>' */