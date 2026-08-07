/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

// Union
export type type_union = string | boolean;


// Array
export type type_array_1 = Array<boolean>;
type type_array_2 = boolean[];


// Array + Union
export type type_union_array_1 = Array<boolean> | Array<string>
type type_union_array_2 = Array<boolean | string>


// Array + Type Alias
export type type_array_alias_1 = Array<type_union>
type type_array_alias_2 = Array<type_union> | type_union
type type_array_alias_3 = Array<type_array_1> | type_array_2


// Tuple
type type_tuple_1 = [string, boolean, number]
type type_tuple_2 = [string, Array<boolean>, number]
type type_tuple_3 = [string, [string, boolean], number]
type type_tuple_4 = [string, boolean[], number]

// Tuple + Union
type type_tuple_union_1 = [string, boolean | null, number]
type type_tuple_union_2 = [string, Array<boolean> | boolean, number]
type type_tuple_union_3 = [string, boolean, number] | Array<boolean> | [string, null, number]
type type_tuple_union_4 = [string, boolean, number] | (boolean | undefined)[]

// Tuple + Type Alias
type type_tuple_alias_1 = [string, boolean | null, type_union_array_1]
type type_tuple_alias_2 = [string, Array<boolean> | boolean, type_union_array_1]
type type_tuple_alias_3 = [string, boolean, number] | Array<boolean> | [string, null, number] | type_tuple_union_4
type type_tuple_alias_4 = [string, boolean, number] | (boolean | undefined)[] | type_tuple_union_3


// Utility
interface Issue {
	title : string
	description : string
}
type type_utility_1 = Partial<Issue>;

interface Issue2 {
	title?: string
	description?: string
}
type type_utility_2 = Required<Issue2>;
type type_utility_3 = Readonly<Issue>;
type type_utility_4 = Record<number, Object>;


function main() {
	let type_union_var : type_union = "name"
	let type_array_var : type_array_1 = new Array<boolean>()
	let type_union_array_var : type_union_array_1 = new Array<string>()
	let type_array_alias_var : type_array_alias_1 = new Array<string | boolean>()
	let type_tuple_var : type_tuple_1 = ["name", true, 2]
	let type_tuple_union_var : type_tuple_union_2 = ["name", new Array<boolean>, 2]
	let type_tuple_alias__var : type_tuple_alias_1 = ["name", null, new Array<string>]
	let type_utility_1_var : type_utility_1 = {}
	let type_utility_2_var : type_utility_2 = {title: "type alias", description: "test for type alias"}
	let type_utility_3_var : type_utility_3 = {title: "type alias", description: "test for type alias"}
	let type_utility_4_var : type_utility_4 = {
		1 : new Object()
	}
}

