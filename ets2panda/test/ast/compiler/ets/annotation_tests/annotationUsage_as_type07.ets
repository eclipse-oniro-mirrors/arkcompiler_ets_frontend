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

class A{
    a:/* @@ label */MyAnno
}

let tuple:[int, string, /* @@ label1 */MyAnno] = [1, "1", new /* @@ label2 */MyAnno()]
let union:int|string|/* @@ label3 */MyAnno = 1

/* @@? 20:21 Error TypeError: Annotations cannot be used as a type. */
/* @@? 23:40 Error TypeError: Annotations cannot be used as a type. */
/* @@? 23:78 Error TypeError: Annotations cannot be used as a type. */
/* @@? 23:78 Error TypeError: Invalid type reference. */
/* @@? 24:37 Error TypeError: Annotations cannot be used as a type. */
