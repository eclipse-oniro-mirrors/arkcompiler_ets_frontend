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

@interface Anno {
   name: string = ""
   id: int = 1
}

@Anno
interface itf0 {
    @Anno
    x1 : int

    @Anno()
    x2 : string

    @Anno({name : "2"})
    x3 : double

    @Anno({id : 1})
    x4 : boolean

    @Anno({name : "2", id : 1})
    x5 : int[]
}

@Anno()
interface itf1 {
}

@Anno({name : "2"})
interface itf2 {
}

@Anno({id : 1})
interface itf3 {
}

@Anno({name : "2", id : 1})
interface itf4 {
}


function main(): void {
}
