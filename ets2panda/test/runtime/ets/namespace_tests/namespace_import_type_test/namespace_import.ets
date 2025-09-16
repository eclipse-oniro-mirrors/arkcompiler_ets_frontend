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

import NS from './namespace_export'
function foo<T extends NS.valueTypeOutside>(data: T){
    arktest.assertEQ(data, 666)
}

function foo2<T extends NS.NS1.valueTypeInside>(data: T){
    arktest.assertEQ(data, "hello");
}

foo(666)
foo2("hello")
