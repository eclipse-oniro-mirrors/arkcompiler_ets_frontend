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
@interface MyAnno {
    a:string = "1"
}

function foo<T>(a:T){}

let array:Array</* @@ label */MyAnno> = new Array</* @@ label1 */MyAnno>()

/* @@? 21:31 Error TypeError: Annotations cannot be used as a type. */
/* @@? 21:45 Error TypeError: Invalid type reference. */
/* @@? 21:66 Error TypeError: Annotations cannot be used as a type. */
