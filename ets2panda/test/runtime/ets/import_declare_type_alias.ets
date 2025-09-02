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

import {type_union, type_array_1 as type_array, type_union_array_1 as type_union_array, type_array_alias_1 as type_array_alias} from './declareTypeAlias_generic.ets'

function main() {
	let type_union_var : type_union<string> = "name"
	let type_array_var : type_array<boolean> = new Array<boolean>()
	let type_union_array_var : type_union_array<string, string> = new Array<string>()
	let type_array_alias_var : type_array_alias<char> = new Array<char | boolean>()
    arktest.assertEQ((typeof type_union_var), "string");
    arktest.assertEQ((typeof type_array_var), "object");
    arktest.assertEQ((typeof type_union_array_var), "object");
    arktest.assertEQ((typeof type_array_alias_var), "object");
}

