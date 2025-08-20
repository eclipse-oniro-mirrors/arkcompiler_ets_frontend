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

namespace NS{
    export function foo(a:number){}
    export function foo2(a:string){}
    export overload /* @@ label */foo{foo2}
}

function foo(a:number){}
function foo2(a:string){}
overload /* @@ label2 */foo{foo2}

/* @@@ label Error TypeError: Method with the same name as overload declaration 'foo', overloadlist must list this medhod. */
/* @@@ label2 Error TypeError: Method with the same name as overload declaration 'foo', overloadlist must list this medhod. */
